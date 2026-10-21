/*
 * SD 卡音乐播放器。界面按 LVGL music demo 的配色和结构来：
 * 左侧深色曲目列表，右侧白色播放页、圆形封面、渐变进度条、上一曲/播放/下一曲。
 *
 * 根目录放 .mp3 或 16-bit PCM 的 .wav。MP3 由 minimp3 在片上解码，
 * 再按 Philips I2S 送给 NS4168。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <stdint.h>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "minimp3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/i2s_std.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "music";

#define TRACKS               32
#define NAME_LEN             64
#define PATH_LEN             288
#define PCM_FRAMES           1024
#define MP3_BUF_BYTES        (16 * 1024)
#define AUDIO_VOLUME_PERCENT 40
#define PROGRESS_MAX         1000
#define SHEET_HANDLE         48
#define BAR_CNT              20

#define COL_BG               0x343247
#define COL_LIST_ON          0x4c4965
#define COL_TITLE            0x504d6d
#define COL_MUTED            0x8a86b8
#define COL_LIST_SUB         0xb1b0be
#define COL_BLUE             0x569af8
#define COL_PURPLE           0xa666f1

typedef struct {
    char name[NAME_LEN];
    char path[PATH_LEN];
    uint32_t rate;
    uint16_t channels;
    uint32_t data_offset;
    uint32_t data_bytes;
    uint32_t duration_sec;
    bool is_mp3;
} track_t;

static track_t s_tracks[TRACKS];
static int s_track_count;
static FILE *s_file;
static i2s_chan_handle_t s_i2s;
static uint32_t s_hw_rate = 48000;
static int16_t *s_pcm;
static uint8_t *s_raw;
static uint8_t *s_mp3_buf;
static int16_t *s_mp3_pcm;
static mp3dec_t s_mp3dec;
static int s_mp3_off;
static int s_mp3_len;
static SemaphoreHandle_t s_wake;
static volatile bool s_paused = true;
static volatile int s_track_req = -1;
static volatile int s_track_index;
static volatile bool s_seek_req;
static volatile uint32_t s_seek_off;
static volatile uint32_t s_played;
static volatile uint16_t s_channels = 2;
static volatile uint32_t s_data_bytes;
static volatile bool s_dragging;

static lv_obj_t *s_list;
static lv_obj_t *s_title;
static lv_obj_t *s_artist;
static lv_obj_t *s_genre;
static lv_obj_t *s_elapsed;
static lv_obj_t *s_total;
static lv_obj_t *s_slider;
static lv_obj_t *s_play_label;
static lv_obj_t *s_cover_letter;
static lv_obj_t *s_rows[TRACKS];
static lv_obj_t *s_row_icon[TRACKS];
static lv_obj_t *s_bars[BAR_CNT];
static int s_shown_track = -2;
static int s_shown_paused = -1;
static uint32_t s_bar_phase;

static const int16_t s_bar_xy[BAR_CNT][2] = {
    {118, 0}, {112, 36}, {95, 69}, {69, 95}, {36, 112},
    {0, 118}, {-36, 112}, {-69, 95}, {-95, 69}, {-112, 36},
    {-118, 0}, {-112, -36}, {-95, -69}, {-69, -95}, {-36, -112},
    {0, -118}, {36, -112}, {69, -95}, {95, -69}, {112, -36},
};

static const uint32_t s_cover_color[] = {
    0x6f8af6, 0xa666f1, 0x8d7adf, 0x569af8,
};

static void format_time(char *buf, size_t len, uint32_t sec)
{
    snprintf(buf, len, "%u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}

static bool ends_with_ci(const char *name, const char *ext)
{
    size_t name_len = strlen(name);
    size_t ext_len = strlen(ext);
    if (name_len < ext_len) {
        return false;
    }
    return strcasecmp(name + (name_len - ext_len), ext) == 0;
}

static void display_name(const char *file_name, char *out, size_t out_len)
{
    snprintf(out, out_len, "%s", file_name);
    size_t n = strlen(out);
    if (n >= 4 && (strcasecmp(out + n - 4, ".wav") == 0 || strcasecmp(out + n - 4, ".mp3") == 0)) {
        out[n - 4] = '\0';
    }
}

static bool parse_wav(FILE *file, track_t *track)
{
    uint8_t hdr[12];
    if (fread(hdr, 1, 12, file) != 12) {
        return false;
    }
    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        return false;
    }

    bool got_fmt = false;
    while (1) {
        char id[4];
        uint32_t size = 0;
        if (fread(id, 1, 4, file) != 4 || fread(&size, 4, 1, file) != 1) {
            return false;
        }
        if (memcmp(id, "fmt ", 4) == 0) {
            uint8_t fmt[16];
            if (size < 16 || fread(fmt, 1, 16, file) != 16) {
                return false;
            }
            uint16_t format = 0;
            uint16_t channels = 0;
            uint16_t bits = 0;
            uint32_t rate = 0;
            memcpy(&format, fmt, 2);
            memcpy(&channels, fmt + 2, 2);
            memcpy(&rate, fmt + 4, 4);
            memcpy(&bits, fmt + 14, 2);
            if (format != 1 || bits != 16 || (channels != 1 && channels != 2) || rate < 8000 || rate > 96000) {
                return false;
            }
            track->rate = rate;
            track->channels = channels;
            if (size > 16 && fseek(file, (long)(size - 16 + (size & 1u)), SEEK_CUR) != 0) {
                return false;
            }
            got_fmt = true;
        } else if (memcmp(id, "data", 4) == 0) {
            long pos = ftell(file);
            if (!got_fmt || pos < 0 || size < 32) {
                return false;
            }
            track->data_offset = (uint32_t)pos;
            track->data_bytes = size;
            uint32_t bytes_per_sec = track->rate * track->channels * 2;
            track->duration_sec = bytes_per_sec ? size / bytes_per_sec : 0;
            return true;
        } else if (fseek(file, (long)(size + (size & 1u)), SEEK_CUR) != 0) {
            return false;
        }
    }
}

static bool parse_mp3(FILE *file, track_t *track)
{
    static const int br_v1_l3[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
    static const int br_v2_l3[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
    static const int sr_v1[] = {44100, 48000, 32000};
    static const int sr_v2[] = {22050, 24000, 16000};
    static const int sr_v25[] = {11025, 12000, 8000};

    if (fseek(file, 0, SEEK_END) != 0) {
        return false;
    }
    long size = ftell(file);
    if (size < 128 || fseek(file, 0, SEEK_SET) != 0) {
        return false;
    }

    uint32_t start = 0;
    uint8_t id3[10];
    if (fread(id3, 1, 10, file) == 10 && memcmp(id3, "ID3", 3) == 0) {
        start = 10u + (((uint32_t)(id3[6] & 0x7f) << 21) | ((uint32_t)(id3[7] & 0x7f) << 14) |
                       ((uint32_t)(id3[8] & 0x7f) << 7) | (uint32_t)(id3[9] & 0x7f));
    }
    if (fseek(file, (long)start, SEEK_SET) != 0) {
        return false;
    }

    uint8_t buf[4096];
    size_t got = fread(buf, 1, sizeof(buf), file);
    for (size_t i = 0; i + 4 < got; i++) {
        if (buf[i] != 0xFF || (buf[i + 1] & 0xE0) != 0xE0) {
            continue;
        }
        int version = (buf[i + 1] >> 3) & 3;
        int layer = (buf[i + 1] >> 1) & 3;
        int br_idx = (buf[i + 2] >> 4) & 0x0F;
        int sr_idx = (buf[i + 2] >> 2) & 3;
        int mode = (buf[i + 3] >> 6) & 3;
        if (layer != 1 || version == 1 || br_idx == 0 || br_idx == 15 || sr_idx == 3) {
            continue;
        }
        int bitrate = (version == 3) ? br_v1_l3[br_idx] : br_v2_l3[br_idx];
        int rate = (version == 3) ? sr_v1[sr_idx] : ((version == 2) ? sr_v2[sr_idx] : sr_v25[sr_idx]);
        if (bitrate <= 0 || rate <= 0) {
            continue;
        }
        track->is_mp3 = true;
        track->rate = (uint32_t)rate;
        track->channels = (mode == 3) ? 1 : 2;
        track->data_offset = 0;
        track->data_bytes = (uint32_t)size;
        track->duration_sec = (uint32_t)((uint64_t)size * 8 / ((uint32_t)bitrate * 1000));
        return track->duration_sec > 0;
    }
    return false;
}

static int scan_tracks(void)
{
    s_track_count = 0;
    DIR *dir = opendir(BSP_SD_MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(TAG, "cannot open SD root");
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && s_track_count < TRACKS) {
        const char *file_name = ent->d_name;
        bool mp3 = ends_with_ci(file_name, ".mp3");
        if (file_name[0] == '.' || (!ends_with_ci(file_name, ".wav") && !mp3)) {
            continue;
        }
        if (snprintf(s_tracks[s_track_count].path, PATH_LEN, "%s/%s", BSP_SD_MOUNT_POINT, file_name) >= PATH_LEN) {
            continue;
        }
        FILE *file = fopen(s_tracks[s_track_count].path, "rb");
        if (file == NULL) {
            continue;
        }
        memset(&s_tracks[s_track_count], 0, sizeof(s_tracks[s_track_count]));
        snprintf(s_tracks[s_track_count].path, PATH_LEN, "%s/%s", BSP_SD_MOUNT_POINT, file_name);
        bool ok = mp3 ? parse_mp3(file, &s_tracks[s_track_count]) : parse_wav(file, &s_tracks[s_track_count]);
        fclose(file);
        if (!ok) {
            ESP_LOGW(TAG, "skip %s", file_name);
            continue;
        }
        display_name(file_name, s_tracks[s_track_count].name, NAME_LEN);
        ESP_LOGI(TAG, "track %s %" PRIu32 " Hz %u ch %u s",
                 s_tracks[s_track_count].name,
                 s_tracks[s_track_count].rate,
                 s_tracks[s_track_count].channels,
                 (unsigned)s_tracks[s_track_count].duration_sec);
        s_track_count++;
    }
    closedir(dir);
    ESP_LOGI(TAG, "%d audio files", s_track_count);
    return s_track_count;
}

static void apply_rate(uint32_t rate)
{
    if (s_i2s == NULL || rate == 0 || rate == s_hw_rate) {
        return;
    }
    esp_err_t err = i2s_channel_disable(s_i2s);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "I2S disable failed");
        return;
    }
    i2s_std_clk_config_t clk = I2S_STD_CLK_DEFAULT_CONFIG(rate);
    err = i2s_channel_reconfig_std_clock(s_i2s, &clk);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "I2S clock %" PRIu32 " failed", rate);
        i2s_channel_enable(s_i2s);
        return;
    }
    i2s_channel_enable(s_i2s);
    s_hw_rate = rate;
    ESP_LOGI(TAG, "I2S %" PRIu32 " Hz", rate);
}

static void flush_audio(void)
{
    if (s_i2s == NULL) {
        return;
    }
    i2s_channel_disable(s_i2s);
    i2s_channel_enable(s_i2s);
}

static void close_file(void)
{
    if (s_file != NULL) {
        fclose(s_file);
        s_file = NULL;
    }
}

static bool open_track(int index)
{
    if (index < 0 || index >= s_track_count) {
        return false;
    }
    close_file();
    s_file = fopen(s_tracks[index].path, "rb");
    if (s_file == NULL || fseek(s_file, s_tracks[index].data_offset, SEEK_SET) != 0) {
        ESP_LOGE(TAG, "cannot open %s", s_tracks[index].name);
        close_file();
        s_paused = true;
        return false;
    }
    apply_rate(s_tracks[index].rate);
    flush_audio();
    if (s_tracks[index].is_mp3) {
        mp3dec_init(&s_mp3dec);
        s_mp3_off = 0;
        s_mp3_len = 0;
    }
    s_track_index = index;
    s_played = 0;
    s_channels = s_tracks[index].channels;
    s_data_bytes = s_tracks[index].data_bytes;
    ESP_LOGI(TAG, "play %s", s_tracks[index].name);
    return true;
}

static void apply_seek(void)
{
    bool mp3 = s_track_index >= 0 && s_track_index < s_track_count && s_tracks[s_track_index].is_mp3;
    if (mp3) {
        uint32_t off = s_seek_off;
        if (off > s_data_bytes) {
            off = s_data_bytes;
        }
        if (s_file != NULL) {
            fseek(s_file, (long)off, SEEK_SET);
            mp3dec_init(&s_mp3dec);
            s_mp3_off = 0;
            s_mp3_len = 0;
            flush_audio();
        }
        s_played = off;
        return;
    }

    uint32_t frame = s_channels ? (uint32_t)s_channels * 2u : 4u;
    uint32_t off = s_seek_off;
    if (frame != 0) {
        off -= off % frame;
    }
    if (off > s_data_bytes) {
        off = s_data_bytes;
    }
    if (s_file != NULL && s_track_index >= 0 && s_track_index < s_track_count) {
        fseek(s_file, s_tracks[s_track_index].data_offset + off, SEEK_SET);
        flush_audio();
    }
    s_played = off;
}

static void write_pcm(const uint8_t *raw, size_t bytes)
{
    if (s_i2s == NULL || s_pcm == NULL || bytes < 2) {
        return;
    }
    const int16_t *src = (const int16_t *)raw;
    size_t frames = (s_channels == 1) ? (bytes / 2) : (bytes / 4);
    size_t offset = 0;
    while (offset < frames && !s_seek_req && s_track_req < 0 && !s_paused) {
        size_t n = frames - offset;
        if (n > PCM_FRAMES) {
            n = PCM_FRAMES;
        }
        for (size_t i = 0; i < n; i++) {
            int32_t left;
            int32_t right;
            if (s_channels == 1) {
                left = right = src[offset + i];
            } else {
                left = src[(offset + i) * 2];
                right = src[(offset + i) * 2 + 1];
            }
            s_pcm[i * 2] = (int16_t)(left * AUDIO_VOLUME_PERCENT / 100);
            s_pcm[i * 2 + 1] = (int16_t)(right * AUDIO_VOLUME_PERCENT / 100);
        }
        size_t written = 0;
        i2s_channel_write(s_i2s, s_pcm, n * 2 * sizeof(int16_t), &written, portMAX_DELAY);
        offset += n;
    }
}

static void mp3_note_played(void)
{
    if (s_file == NULL) {
        return;
    }
    long pos = ftell(s_file);
    int buffered = s_mp3_len - s_mp3_off;
    if (pos >= buffered && buffered >= 0) {
        s_played = (uint32_t)(pos - buffered);
    }
}

static bool mp3_refill(void)
{
    int avail = s_mp3_len - s_mp3_off;
    if (avail < 0) {
        avail = 0;
    }
    if (s_mp3_off > 0 && avail > 0) {
        memmove(s_mp3_buf, s_mp3_buf + s_mp3_off, (size_t)avail);
    }
    s_mp3_len = avail;
    s_mp3_off = 0;
    if (s_file == NULL || s_mp3_buf == NULL) {
        return s_mp3_len > 0;
    }
    size_t space = (size_t)(MP3_BUF_BYTES - s_mp3_len);
    if (space == 0) {
        return s_mp3_len > 0;
    }
    size_t n = fread(s_mp3_buf + s_mp3_len, 1, space, s_file);
    s_mp3_len += (int)n;
    return n > 0 || s_mp3_len > 0;
}

static bool play_one_mp3_frame(void)
{
    if (s_mp3_buf == NULL || s_mp3_pcm == NULL) {
        return false;
    }
    if (s_mp3_len - s_mp3_off < 2048 && !mp3_refill() && s_mp3_len - s_mp3_off < 4) {
        return false;
    }

    mp3dec_frame_info_t info;
    memset(&info, 0, sizeof(info));
    int samples = mp3dec_decode_frame(&s_mp3dec, s_mp3_buf + s_mp3_off,
                                      s_mp3_len - s_mp3_off, s_mp3_pcm, &info);
    if (info.frame_bytes > 0) {
        s_mp3_off += info.frame_bytes;
        mp3_note_played();
    } else if (s_mp3_len < MP3_BUF_BYTES && s_file != NULL && !feof(s_file)) {
        int before = s_mp3_len;
        if (!mp3_refill() || s_mp3_len == before) {
            if (s_mp3_len - s_mp3_off > 0) {
                s_mp3_off++;
                return true;
            }
            return false;
        }
        return true;
    } else if (s_mp3_len - s_mp3_off > 0) {
        s_mp3_off++;
        return true;
    } else {
        return false;
    }

    if (samples > 0 && info.channels > 0 && info.hz > 0) {
        s_channels = (uint16_t)info.channels;
        apply_rate((uint32_t)info.hz);
        write_pcm((const uint8_t *)s_mp3_pcm, (size_t)samples * (size_t)info.channels * sizeof(int16_t));
    }
    return true;
}

static void player_task(void *arg)
{
    (void)arg;
    while (true) {
        if (s_track_req >= 0) {
            int index = s_track_req;
            s_track_req = -1;
            open_track(index);
            continue;
        }
        if (s_seek_req) {
            s_seek_req = false;
            apply_seek();
        }
        if (s_paused || s_file == NULL) {
            xSemaphoreTake(s_wake, portMAX_DELAY);
            continue;
        }

        bool mp3 = s_track_index >= 0 && s_track_index < s_track_count && s_tracks[s_track_index].is_mp3;
        if (mp3) {
            bool more = false;
            for (int i = 0; i < 4 && !s_seek_req && s_track_req < 0 && !s_paused; i++) {
                if (!play_one_mp3_frame()) {
                    more = false;
                    break;
                }
                more = true;
            }
            if (!more && !s_seek_req && s_track_req < 0 && !s_paused) {
                int next = s_track_count > 0 ? (s_track_index + 1) % s_track_count : 0;
                open_track(next);
            }
            continue;
        }

        uint32_t frame = s_channels * 2u;
        if (frame == 0 || s_played + frame > s_data_bytes) {
            int next = s_track_count > 0 ? (s_track_index + 1) % s_track_count : 0;
            open_track(next);
            continue;
        }

        uint32_t remain = s_data_bytes - s_played;
        size_t want = PCM_FRAMES * frame;
        if (want > remain) {
            want = remain - (remain % frame);
        }
        size_t got = fread(s_raw, 1, want, s_file);
        got -= got % frame;
        if (got < frame) {
            int next = (s_track_index + 1) % s_track_count;
            open_track(next);
            continue;
        }
        write_pcm(s_raw, got);
        if (!s_seek_req && s_track_req < 0) {
            s_played += (uint32_t)got;
        }
    }
}

static void request_track(int index, bool play)
{
    if (index < 0 || index >= s_track_count) {
        return;
    }
    s_track_req = index;
    if (play) {
        s_paused = false;
    }
    if (s_wake != NULL) {
        xSemaphoreGive(s_wake);
    }
}

static void play_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_track_count <= 0) {
        return;
    }
    s_paused = !s_paused;
    if (!s_paused && s_file == NULL) {
        request_track(s_track_index, true);
        return;
    }
    if (s_wake != NULL) {
        xSemaphoreGive(s_wake);
    }
}

static void prev_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_track_count <= 0) {
        return;
    }
    int index = s_track_index - 1;
    if (index < 0) {
        index = s_track_count - 1;
    }
    request_track(index, true);
}

static void next_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED || s_track_count <= 0) {
        return;
    }
    request_track((s_track_index + 1) % s_track_count, true);
}

static void list_event(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
        return;
    }
    request_track((int)(intptr_t)lv_event_get_user_data(e), true);
}

static uint32_t seconds_at(int index, uint32_t bytes)
{
    if (index < 0 || index >= s_track_count) {
        return 0;
    }
    if (s_tracks[index].is_mp3) {
        uint32_t total = s_tracks[index].data_bytes;
        if (total == 0) {
            return 0;
        }
        return (uint32_t)((uint64_t)bytes * s_tracks[index].duration_sec / total);
    }
    uint32_t bps = s_tracks[index].rate * s_tracks[index].channels * 2u;
    return bps ? bytes / bps : 0;
}

static void slider_event(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED) {
        s_dragging = true;
        return;
    }
    if (s_track_count <= 0 || s_track_index < 0) {
        return;
    }
    uint32_t bytes = s_tracks[s_track_index].data_bytes;
    if (code == LV_EVENT_VALUE_CHANGED && s_dragging) {
        uint32_t off = (uint32_t)((uint64_t)bytes * lv_slider_get_value(s_slider) / PROGRESS_MAX);
        char buf[16];
        format_time(buf, sizeof(buf), seconds_at(s_track_index, off));
        lv_label_set_text(s_elapsed, buf);
        return;
    }
    if (code != LV_EVENT_RELEASED) {
        return;
    }
    s_dragging = false;
    s_seek_off = (uint32_t)((uint64_t)bytes * lv_slider_get_value(s_slider) / PROGRESS_MAX);
    s_seek_req = true;
    if (s_paused && s_wake != NULL) {
        xSemaphoreGive(s_wake);
    }
}

static lv_obj_t *round_btn(lv_obj_t *parent, int size, const char *symbol, lv_event_cb_t cb, bool primary)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(primary ? COL_BLUE : 0xf3f1fb), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(primary ? COL_PURPLE : 0xe4def6), LV_STATE_PRESSED);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, symbol);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label, primary ? lv_color_white() : lv_color_hex(COL_TITLE), 0);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return label;
}

static void build_list(lv_obj_t *parent)
{
    static lv_style_t style_btn;
    static lv_style_t style_on;
    static const int32_t cols[] = {36, LV_GRID_FR(1), 72, LV_GRID_TEMPLATE_LAST};
    static const int32_t rows[] = {28, 22, LV_GRID_TEMPLATE_LAST};

    lv_style_init(&style_btn);
    lv_style_set_bg_opa(&style_btn, LV_OPA_TRANSP);
    lv_style_set_layout(&style_btn, LV_LAYOUT_GRID);
    lv_style_set_grid_column_dsc_array(&style_btn, cols);
    lv_style_set_grid_row_dsc_array(&style_btn, rows);
    lv_style_set_grid_row_align(&style_btn, LV_GRID_ALIGN_CENTER);
    lv_style_set_pad_left(&style_btn, 16);
    lv_style_set_pad_right(&style_btn, 16);

    lv_style_init(&style_on);
    lv_style_set_bg_opa(&style_on, LV_OPA_COVER);
    lv_style_set_bg_color(&style_on, lv_color_hex(COL_LIST_ON));

    for (int i = 0; i < s_track_count; i++) {
        lv_obj_t *btn = lv_obj_create(parent);
        lv_obj_remove_style_all(btn);
        lv_obj_set_size(btn, lv_pct(100), 78);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_on, LV_STATE_CHECKED);
        lv_obj_add_style(btn, &style_on, LV_STATE_PRESSED);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(btn, list_event, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        s_row_icon[i] = lv_label_create(btn);
        lv_label_set_text(s_row_icon[i], LV_SYMBOL_PLAY);
        lv_obj_set_style_text_color(s_row_icon[i], lv_color_white(), 0);
        lv_obj_set_style_text_font(s_row_icon[i], &lv_font_montserrat_16, 0);
        lv_obj_set_grid_cell(s_row_icon[i], LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_CENTER, 0, 2);

        lv_obj_t *title = lv_label_create(btn);
        lv_label_set_text(title, s_tracks[i].name);
        lv_label_set_long_mode(title, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(title, 820);
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
        lv_obj_set_grid_cell(title, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);

        lv_obj_t *artist = lv_label_create(btn);
        lv_label_set_text(artist, s_tracks[i].is_mp3 ? "MP3" : "WAV");
        lv_obj_set_style_text_color(artist, lv_color_hex(COL_LIST_SUB), 0);
        lv_obj_set_style_text_font(artist, &lv_font_montserrat_14, 0);
        lv_obj_set_grid_cell(artist, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);

        char time_buf[16];
        format_time(time_buf, sizeof(time_buf), s_tracks[i].duration_sec);
        lv_obj_t *time = lv_label_create(btn);
        lv_label_set_text(time, time_buf);
        lv_obj_set_style_text_color(time, lv_color_white(), 0);
        lv_obj_set_style_text_font(time, &lv_font_montserrat_16, 0);
        lv_obj_set_grid_cell(time, LV_GRID_ALIGN_END, 2, 1, LV_GRID_ALIGN_CENTER, 0, 2);

        lv_obj_t *border = lv_obj_create(btn);
        lv_obj_remove_style_all(border);
        lv_obj_set_size(border, lv_pct(100), 1);
        lv_obj_set_style_bg_color(border, lv_color_hex(0x4a4760), 0);
        lv_obj_set_style_bg_opa(border, LV_OPA_COVER, 0);
        lv_obj_align(border, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_flag(border, LV_OBJ_FLAG_IGNORE_LAYOUT);
        s_rows[i] = btn;
    }
}

static void refresh_ui(lv_timer_t *timer)
{
    (void)timer;
    bool paused = s_paused;
    int track = s_track_index;
    if (track < 0 || track >= s_track_count) {
        track = -1;
    }

    if (track != s_shown_track || (int)paused != s_shown_paused) {
        s_shown_track = track;
        s_shown_paused = paused;
        lv_label_set_text(s_play_label, paused ? LV_SYMBOL_PLAY : LV_SYMBOL_PAUSE);
        if (track >= 0) {
            lv_label_set_text(s_title, s_tracks[track].name);
            char genre[32];
            snprintf(genre, sizeof(genre), "%s  %u kHz %s",
                     s_tracks[track].is_mp3 ? "MP3" : "WAV",
                     (unsigned)((s_tracks[track].rate + 500) / 1000),
                     s_tracks[track].channels == 1 ? "mono" : "stereo");
            lv_label_set_text(s_genre, genre);
            char letter[2] = {s_tracks[track].name[0] ? s_tracks[track].name[0] : '?', '\0'};
            lv_label_set_text(s_cover_letter, letter);
            lv_obj_set_style_bg_color(lv_obj_get_parent(s_cover_letter),
                                      lv_color_hex(s_cover_color[track % 4]), 0);
            char total[16];
            format_time(total, sizeof(total), s_tracks[track].duration_sec);
            lv_label_set_text(s_total, total);
        }
        for (int i = 0; i < s_track_count; i++) {
            bool on = i == track;
            if (on) {
                lv_obj_add_state(s_rows[i], LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(s_rows[i], LV_STATE_CHECKED);
            }
            lv_label_set_text(s_row_icon[i], (on && !paused) ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
        }
    }

    if (!s_dragging && track >= 0 && s_tracks[track].data_bytes > 0) {
        uint32_t played = s_played;
        int32_t permille = (int32_t)((uint64_t)played * PROGRESS_MAX / s_tracks[track].data_bytes);
        lv_slider_set_value(s_slider, permille, LV_ANIM_OFF);
        char elapsed[16];
        format_time(elapsed, sizeof(elapsed), seconds_at(track, played));
        lv_label_set_text(s_elapsed, elapsed);
    }

    s_bar_phase++;
    for (int i = 0; i < BAR_CNT; i++) {
        int height = 8;
        if (!paused && track >= 0) {
            height = 10 + (int)((s_bar_phase + (uint32_t)i * 3) % 28);
        }
        lv_obj_set_size(s_bars[i], 6, height);
        lv_obj_set_pos(s_bars[i], 140 + s_bar_xy[i][0] - 3, 140 + s_bar_xy[i][1] - height / 2);
    }
}

static void fill_list(void)
{
    if (s_list == NULL) {
        return;
    }
    lv_obj_clean(s_list);
    if (s_track_count == 0) {
        lv_obj_t *empty = lv_label_create(s_list);
        lv_label_set_text(empty, "No WAV or MP3 in the SD root.");
        lv_obj_set_style_text_color(empty, lv_color_white(), 0);
        lv_obj_set_style_text_font(empty, &lv_font_montserrat_16, 0);
        lv_obj_set_style_pad_left(empty, 28, 0);
        lv_obj_set_style_pad_top(empty, 24, 0);
        return;
    }
    build_list(s_list);
}

static void create_screen(void)
{
    const int32_t screen_h = BSP_LCD_V_RES;
    bsp_display_lock(0);
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(COL_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    s_list = lv_obj_create(scr);
    lv_obj_remove_style_all(s_list);
    lv_obj_set_pos(s_list, 0, SHEET_HANDLE);
    lv_obj_set_size(s_list, BSP_LCD_H_RES, screen_h - SHEET_HANDLE);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_list, 0, 0);
    lv_obj_set_scrollbar_mode(s_list, LV_SCROLLBAR_MODE_AUTO);
    fill_list();

    /* 曲目列表在底层。白色播放页盖住它，上滑后吸附，只留顶部一条白边。 */
    lv_obj_t *sheet = lv_obj_create(scr);
    lv_obj_remove_style_all(sheet);
    lv_obj_set_size(sheet, lv_pct(100), lv_pct(100));
    lv_obj_remove_flag(sheet, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_scrollbar_mode(sheet, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(sheet, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(sheet, LV_SCROLL_SNAP_CENTER);

    lv_obj_t *player = lv_obj_create(sheet);
    lv_obj_set_y(player, -SHEET_HANDLE);
    lv_obj_set_size(player, BSP_LCD_H_RES, screen_h + SHEET_HANDLE * 2);
    lv_obj_set_style_bg_color(player, lv_color_white(), 0);
    lv_obj_set_style_border_width(player, 0, 0);
    lv_obj_set_style_radius(player, 0, 0);
    lv_obj_set_style_pad_all(player, 0, 0);
    lv_obj_remove_flag(player, LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(player, LV_DIR_VER);

    lv_obj_t *placeholder1 = lv_obj_create(sheet);
    lv_obj_remove_style_all(placeholder1);
    lv_obj_remove_flag(placeholder1, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(placeholder1, lv_pct(100), screen_h);
    lv_obj_set_y(placeholder1, 0);

    lv_obj_t *placeholder2 = lv_obj_create(sheet);
    lv_obj_remove_style_all(placeholder2);
    lv_obj_remove_flag(placeholder2, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(placeholder2, lv_pct(100), screen_h - SHEET_HANDLE * 2);
    lv_obj_set_y(placeholder2, screen_h + SHEET_HANDLE);

    lv_obj_t *body = lv_obj_create(player);
    lv_obj_remove_style_all(body);
    lv_obj_set_pos(body, 0, SHEET_HANDLE);
    lv_obj_set_size(body, BSP_LCD_H_RES, screen_h - SHEET_HANDLE);
    lv_obj_remove_flag(body, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *stage = lv_obj_create(body);
    lv_obj_remove_style_all(stage);
    lv_obj_set_size(stage, 280, 280);
    lv_obj_align(stage, LV_ALIGN_TOP_MID, 0, 8);
    for (int i = 0; i < BAR_CNT; i++) {
        s_bars[i] = lv_obj_create(stage);
        lv_obj_remove_style_all(s_bars[i]);
        lv_obj_set_style_bg_color(s_bars[i], lv_color_hex(i % 3 == 0 ? 0xe9dbfc : (i % 3 == 1 ? COL_BLUE : 0xffffff)), 0);
        lv_obj_set_style_bg_opa(s_bars[i], LV_OPA_COVER, 0);
        lv_obj_set_style_radius(s_bars[i], 3, 0);
        lv_obj_remove_flag(s_bars[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t *cover = lv_obj_create(stage);
    lv_obj_set_size(cover, 148, 148);
    lv_obj_align(cover, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(cover, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cover, lv_color_hex(s_cover_color[0]), 0);
    lv_obj_set_style_border_width(cover, 0, 0);
    lv_obj_set_style_shadow_width(cover, 0, 0);
    lv_obj_remove_flag(cover, LV_OBJ_FLAG_SCROLLABLE);
    s_cover_letter = lv_label_create(cover);
    lv_label_set_text(s_cover_letter, s_track_count > 0 ? "" : "?");
    lv_obj_set_style_text_color(s_cover_letter, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_cover_letter, &lv_font_montserrat_24, 0);
    lv_obj_center(s_cover_letter);

    s_title = lv_label_create(body);
    lv_label_set_text(s_title, s_track_count > 0 ? s_tracks[0].name : "No music");
    lv_label_set_long_mode(s_title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(s_title, 640);
    lv_obj_set_style_text_align(s_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(s_title, lv_color_hex(COL_TITLE), 0);
    lv_obj_set_style_text_font(s_title, &lv_font_montserrat_24, 0);
    lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 292);

    s_artist = lv_label_create(body);
    lv_label_set_text(s_artist, "SD card");
    lv_obj_set_style_text_color(s_artist, lv_color_hex(COL_TITLE), 0);
    lv_obj_set_style_text_font(s_artist, &lv_font_montserrat_16, 0);
    lv_obj_align(s_artist, LV_ALIGN_TOP_MID, 0, 328);

    s_genre = lv_label_create(body);
    lv_label_set_text(s_genre, "MP3 or WAV");
    lv_obj_set_style_text_color(s_genre, lv_color_hex(COL_MUTED), 0);
    lv_obj_set_style_text_font(s_genre, &lv_font_montserrat_14, 0);
    lv_obj_align(s_genre, LV_ALIGN_TOP_MID, 0, 354);

    s_elapsed = lv_label_create(body);
    lv_label_set_text(s_elapsed, "0:00");
    lv_obj_set_style_text_color(s_elapsed, lv_color_hex(COL_MUTED), 0);
    lv_obj_set_style_text_font(s_elapsed, &lv_font_montserrat_14, 0);
    lv_obj_align(s_elapsed, LV_ALIGN_TOP_LEFT, 36, 400);

    s_total = lv_label_create(body);
    lv_label_set_text(s_total, "0:00");
    lv_obj_set_style_text_color(s_total, lv_color_hex(COL_MUTED), 0);
    lv_obj_set_style_text_font(s_total, &lv_font_montserrat_14, 0);
    lv_obj_align(s_total, LV_ALIGN_TOP_RIGHT, -36, 400);

    s_slider = lv_slider_create(body);
    lv_obj_set_size(s_slider, BSP_LCD_H_RES - 220, 6);
    lv_obj_align(s_slider, LV_ALIGN_TOP_MID, 0, 406);
    lv_slider_set_range(s_slider, 0, PROGRESS_MAX);
    lv_obj_set_style_bg_grad_dir(s_slider, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(COL_BLUE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(s_slider, lv_color_hex(COL_PURPLE), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_slider, lv_color_hex(COL_PURPLE), LV_PART_KNOB);
    lv_obj_set_style_pad_all(s_slider, 8, LV_PART_KNOB);
    lv_obj_add_event_cb(s_slider, slider_event, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_slider, slider_event, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_slider, slider_event, LV_EVENT_RELEASED, NULL);

    lv_obj_t *controls = lv_obj_create(body);
    lv_obj_remove_style_all(controls);
    lv_obj_set_size(controls, 280, 84);
    lv_obj_align(controls, LV_ALIGN_TOP_MID, 0, 448);
    lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(controls, LV_OBJ_FLAG_SCROLLABLE);
    round_btn(controls, 56, LV_SYMBOL_PREV, prev_event, false);
    s_play_label = round_btn(controls, 72, LV_SYMBOL_PLAY, play_event, true);
    round_btn(controls, 56, LV_SYMBOL_NEXT, next_event, false);

    lv_obj_t *handle = lv_obj_create(player);
    lv_obj_remove_style_all(handle);
    lv_obj_set_pos(handle, 0, screen_h);
    lv_obj_set_size(handle, BSP_LCD_H_RES, SHEET_HANDLE);
    lv_obj_set_flex_flow(handle, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(handle, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(handle, 6, 0);
    lv_obj_remove_flag(handle, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *handle_label = lv_label_create(handle);
    lv_label_set_text(handle_label, "ALL TRACKS");
    lv_obj_set_style_text_color(handle_label, lv_color_hex(COL_MUTED), 0);
    lv_obj_set_style_text_font(handle_label, &lv_font_montserrat_14, 0);
    lv_obj_t *handle_bar = lv_obj_create(handle);
    lv_obj_set_size(handle_bar, 40, 3);
    lv_obj_set_style_bg_color(handle_bar, lv_color_hex(COL_MUTED), 0);
    lv_obj_set_style_bg_opa(handle_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(handle_bar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(handle_bar, 0, 0);
    lv_obj_remove_flag(handle_bar, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_update_layout(sheet);
    lv_obj_scroll_to_y(sheet, 0, LV_ANIM_OFF);
    lv_timer_create(refresh_ui, 100, NULL);
    bsp_display_unlock();
}

esp_err_t music_player_start(void)
{
    create_screen();

    esp_err_t err = bsp_sdcard_mount();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(err));
        if (bsp_display_lock(100)) {
            lv_label_set_text(s_title, "SD card mount failed");
            bsp_display_unlock();
        }
        return err;
    }

    bsp_audio_codec_speaker_init();
    s_i2s = bsp_audio_get_speaker_i2s_chan();
    if (s_i2s == NULL) {
        ESP_LOGW(TAG, "speaker I2S is not ready");
    }

    s_pcm = heap_caps_malloc(PCM_FRAMES * 2 * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_raw = heap_caps_malloc(PCM_FRAMES * 4, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_mp3_buf = heap_caps_malloc(MP3_BUF_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_mp3_pcm = heap_caps_malloc(MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(int16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (s_pcm == NULL || s_raw == NULL || s_mp3_buf == NULL || s_mp3_pcm == NULL) {
        return ESP_ERR_NO_MEM;
    }

    scan_tracks();
    if (bsp_display_lock(200)) {
        fill_list();
        if (s_track_count == 0) {
            lv_label_set_text(s_title, "No music");
        } else {
            lv_label_set_text(s_title, s_tracks[0].name);
        }
        bsp_display_unlock();
    }

    s_wake = xSemaphoreCreateBinary();
    if (s_wake == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (s_track_count > 0) {
        s_track_index = 0;
        s_track_req = 0;
        s_paused = false;
    }
    if (xTaskCreatePinnedToCore(player_task, "music", 32768, NULL, 5, NULL, 1) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    if (!s_paused && s_wake != NULL) {
        xSemaphoreGive(s_wake);
    }
    return ESP_OK;
}
