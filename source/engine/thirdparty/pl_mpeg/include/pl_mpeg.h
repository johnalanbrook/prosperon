#ifndef PL_MPEG_H
#define PL_MPEG_H

#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Public Data Types


// Object types for the various interfaces

typedef struct plm_t plm_t;
typedef struct plm_buffer_t plm_buffer_t;
typedef struct plm_demux_t plm_demux_t;
typedef struct plm_video_t plm_video_t;
typedef struct plm_audio_t plm_audio_t;


// Demuxed MPEG PS packet
// The type maps directly to the various MPEG-PES start codes. PTS is the
// presentation time stamp of the packet in seconds. Note that not all packets
// have a PTS value, indicated by PLM_PACKET_INVALID_TS.

#define PLM_PACKET_INVALID_TS -1

typedef struct {
	int type;
	double pts;
	size_t length;
	uint8_t *data;
} plm_packet_t;


// Decoded Video Plane
// The byte length of the data is width * height. Note that different planes
// have different sizes: the Luma plane (Y) is double the size of each of
// the two Chroma planes (Cr, Cb) - i.e. 4 times the byte length.
// Also note that the size of the plane does *not* denote the size of the
// displayed frame. The sizes of planes are always rounded up to the nearest
// macroblock (16px).

typedef struct {
	unsigned int width;
	unsigned int height;
	uint8_t *data;
} plm_plane_t;


// Decoded Video Frame
// width and height denote the desired display size of the frame. This may be
// different from the internal size of the 3 planes.

typedef struct {
	double time;
	unsigned int width;
	unsigned int height;
	plm_plane_t y;
	plm_plane_t cr;
	plm_plane_t cb;
} plm_frame_t;


// Callback function type for decoded video frames used by the high-level
// plm_* interface

typedef void(*plm_video_decode_callback)
	(plm_t *self, plm_frame_t *frame, void *user);


// Decoded Audio Samples
// Samples are stored as normalized (-1, 1) float either interleaved, or if
// PLM_AUDIO_SEPARATE_CHANNELS is defined, in two separate arrays.
// The `count` is always PLM_AUDIO_SAMPLES_PER_FRAME and just there for
// convenience.

#define PLM_AUDIO_SAMPLES_PER_FRAME 1152

typedef struct {
	double time;
	unsigned int count;
	#ifdef PLM_AUDIO_SEPARATE_CHANNELS
		float left[PLM_AUDIO_SAMPLES_PER_FRAME];
		float right[PLM_AUDIO_SAMPLES_PER_FRAME];
	#else
		float interleaved[PLM_AUDIO_SAMPLES_PER_FRAME * 2];
	#endif
} plm_samples_t;


// Callback function type for decoded audio samples used by the high-level
// plm_* interface

typedef void(*plm_audio_decode_callback)
	(plm_t *self, plm_samples_t *samples, void *user);


// Callback function for plm_buffer when it needs more data

typedef void(*plm_buffer_load_callback)(plm_buffer_t *self, void *user);



// -----------------------------------------------------------------------------
// plm_* public API
// High-Level API for loading/demuxing/decoding MPEG-PS data


// Create a plmpeg instance with a filename. Returns NULL if the file could not
// be opened.

plm_t *plm_create_with_filename(const char *filename);


// Create a plmpeg instance with a file handle. Pass TRUE to close_when_done to
// let plmpeg call fclose() on the handle when plm_destroy() is called.

plm_t *plm_create_with_file(FILE *fh, int close_when_done);


// Create a plmpeg instance with a pointer to memory as source. This assumes the
// whole file is in memory. The memory is not copied. Pass TRUE to
// free_when_done to let plmpeg call free() on the pointer when plm_destroy()
// is called.

plm_t *plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);


// Create a plmpeg instance with a plm_buffer as source. Pass TRUE to
// destroy_when_done to let plmpeg call plm_buffer_destroy() on the buffer when
// plm_destroy() is called.

plm_t *plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);


// Destroy a plmpeg instance and free all data.

void plm_destroy(plm_t *self);


// Get whether we have headers on all available streams and we can accurately
// report the number of video/audio streams, video dimensions, framerate and
// audio samplerate.
// This returns FALSE if the file is not an MPEG-PS file or - when not using a
// file as source - when not enough data is available yet.

int plm_has_headers(plm_t *self);


// Get or set whether video decoding is enabled. Default TRUE.

int plm_get_video_enabled(plm_t *self);
void plm_set_video_enabled(plm_t *self, int enabled);


// Get the number of video streams (0--1) reported in the system header.

int plm_get_num_video_streams(plm_t *self);


// Get the display width/height of the video stream.

int plm_get_width(plm_t *self);
int plm_get_height(plm_t *self);


// Get the framerate of the video stream in frames per second.

double plm_get_framerate(plm_t *self);


// Get or set whether audio decoding is enabled. Default TRUE.

int plm_get_audio_enabled(plm_t *self);
void plm_set_audio_enabled(plm_t *self, int enabled);


// Get the number of audio streams (0--4) reported in the system header.

int plm_get_num_audio_streams(plm_t *self);


// Set the desired audio stream (0--3). Default 0.

void plm_set_audio_stream(plm_t *self, int stream_index);


// Get the samplerate of the audio stream in samples per second.

int plm_get_samplerate(plm_t *self);


// Get or set the audio lead time in seconds - the time in which audio samples
// are decoded in advance (or behind) the video decode time. Typically this
// should be set to the duration of the buffer of the audio API that you use
// for output. E.g. for SDL2: (SDL_AudioSpec.samples / samplerate)

double plm_get_audio_lead_time(plm_t *self);
void plm_set_audio_lead_time(plm_t *self, double lead_time);


// Get the current internal time in seconds.

double plm_get_time(plm_t *self);


// Get the video duration of the underlying source in seconds.

double plm_get_duration(plm_t *self);


// Rewind all buffers back to the beginning.

void plm_rewind(plm_t *self);


// Get or set looping. Default FALSE.

int plm_get_loop(plm_t *self);
void plm_set_loop(plm_t *self, int loop);


// Get whether the file has ended. If looping is enabled, this will always
// return FALSE.

int plm_has_ended(plm_t *self);


// Set the callback for decoded video frames used with plm_decode(). If no
// callback is set, video data will be ignored and not be decoded. The *user
// Parameter will be passed to your callback.

void plm_set_video_decode_callback(plm_t *self, plm_video_decode_callback fp, void *user);


// Set the callback for decoded audio samples used with plm_decode(). If no
// callback is set, audio data will be ignored and not be decoded. The *user
// Parameter will be passed to your callback.

void plm_set_audio_decode_callback(plm_t *self, plm_audio_decode_callback fp, void *user);


// Advance the internal timer by seconds and decode video/audio up to this time.
// This will call the video_decode_callback and audio_decode_callback any number
// of times. A frame-skip is not implemented, i.e. everything up to current time
// will be decoded.

void plm_decode(plm_t *self, double seconds);


// Decode and return one video frame. Returns NULL if no frame could be decoded
// (either because the source ended or data is corrupt). If you only want to
// decode video, you should disable audio via plm_set_audio_enabled().
// The returned plm_frame_t is valid until the next call to plm_decode_video()
// or until plm_destroy() is called.

plm_frame_t *plm_decode_video(plm_t *self);


// Decode and return one audio frame. Returns NULL if no frame could be decoded
// (either because the source ended or data is corrupt). If you only want to
// decode audio, you should disable video via plm_set_video_enabled().
// The returned plm_samples_t is valid until the next call to plm_decode_audio()
// or until plm_destroy() is called.

plm_samples_t *plm_decode_audio(plm_t *self);


// Seek to the specified time, clamped between 0 -- duration. This can only be
// used when the underlying plm_buffer is seekable, i.e. for files, fixed
// memory buffers or _for_appending buffers.
// If seek_exact is TRUE this will seek to the exact time, otherwise it will
// seek to the last intra frame just before the desired time. Exact seeking can
// be slow, because all frames up to the seeked one have to be decoded on top of
// the previous intra frame.
// If seeking succeeds, this function will call the video_decode_callback
// exactly once with the target frame. If audio is enabled, it will also call
// the audio_decode_callback any number of times, until the audio_lead_time is
// satisfied.
// Returns TRUE if seeking succeeded or FALSE if no frame could be found.

int plm_seek(plm_t *self, double time, int seek_exact);


// Similar to plm_seek(), but will not call the video_decode_callback,
// audio_decode_callback or make any attempts to sync audio.
// Returns the found frame or NULL if no frame could be found.

plm_frame_t *plm_seek_frame(plm_t *self, double time, int seek_exact);



// -----------------------------------------------------------------------------
// plm_buffer public API
// Provides the data source for all other plm_* interfaces


// The default size for buffers created from files or by the high-level API

#ifndef PLM_BUFFER_DEFAULT_SIZE
#define PLM_BUFFER_DEFAULT_SIZE (128 * 1024)
#endif


// Create a buffer instance with a filename. Returns NULL if the file could not
// be opened.

plm_buffer_t *plm_buffer_create_with_filename(const char *filename);


// Create a buffer instance with a file handle. Pass TRUE to close_when_done
// to let plmpeg call fclose() on the handle when plm_destroy() is called.

plm_buffer_t *plm_buffer_create_with_file(FILE *fh, int close_when_done);


// Create a buffer instance with a pointer to memory as source. This assumes
// the whole file is in memory. The bytes are not copied. Pass 1 to
// free_when_done to let plmpeg call free() on the pointer when plm_destroy()
// is called.

plm_buffer_t *plm_buffer_create_with_memory(uint8_t *bytes, size_t length, int free_when_done);


// Create an empty buffer with an initial capacity. The buffer will grow
// as needed. Data that has already been read, will be discarded.

plm_buffer_t *plm_buffer_create_with_capacity(size_t capacity);


// Create an empty buffer with an initial capacity. The buffer will grow
// as needed. Decoded data will *not* be discarded. This can be used when
// loading a file over the network, without needing to throttle the download.
// It also allows for seeking in the already loaded data.

plm_buffer_t *plm_buffer_create_for_appending(size_t initial_capacity);


// Destroy a buffer instance and free all data

void plm_buffer_destroy(plm_buffer_t *self);


// Copy data into the buffer. If the data to be written is larger than the
// available space, the buffer will realloc() with a larger capacity.
// Returns the number of bytes written. This will always be the same as the
// passed in length, except when the buffer was created _with_memory() for
// which _write() is forbidden.

size_t plm_buffer_write(plm_buffer_t *self, uint8_t *bytes, size_t length);


// Mark the current byte length as the end of this buffer and signal that no
// more data is expected to be written to it. This function should be called
// just after the last plm_buffer_write().
// For _with_capacity buffers, this is cleared on a plm_buffer_rewind().

void plm_buffer_signal_end(plm_buffer_t *self);


// Set a callback that is called whenever the buffer needs more data

void plm_buffer_set_load_callback(plm_buffer_t *self, plm_buffer_load_callback fp, void *user);


// Rewind the buffer back to the beginning. When loading from a file handle,
// this also seeks to the beginning of the file.

void plm_buffer_rewind(plm_buffer_t *self);


// Get the total size. For files, this returns the file size. For all other
// types it returns the number of bytes currently in the buffer.

size_t plm_buffer_get_size(plm_buffer_t *self);


// Get the number of remaining (yet unread) bytes in the buffer. This can be
// useful to throttle writing.

size_t plm_buffer_get_remaining(plm_buffer_t *self);


// Get whether the read position of the buffer is at the end and no more data
// is expected.

int plm_buffer_has_ended(plm_buffer_t *self);



// -----------------------------------------------------------------------------
// plm_demux public API
// Demux an MPEG Program Stream (PS) data into separate packages


// Various Packet Types

static const int PLM_DEMUX_PACKET_PRIVATE = 0xBD;
static const int PLM_DEMUX_PACKET_AUDIO_1 = 0xC0;
static const int PLM_DEMUX_PACKET_AUDIO_2 = 0xC1;
static const int PLM_DEMUX_PACKET_AUDIO_3 = 0xC2;
static const int PLM_DEMUX_PACKET_AUDIO_4 = 0xC2;
static const int PLM_DEMUX_PACKET_VIDEO_1 = 0xE0;


// Create a demuxer with a plm_buffer as source. This will also attempt to read
// the pack and system headers from the buffer.

plm_demux_t *plm_demux_create(plm_buffer_t *buffer, int destroy_when_done);


// Destroy a demuxer and free all data.

void plm_demux_destroy(plm_demux_t *self);


// Returns TRUE/FALSE whether pack and system headers have been found. This will
// attempt to read the headers if non are present yet.

int plm_demux_has_headers(plm_demux_t *self);


// Returns the number of video streams found in the system header. This will
// attempt to read the system header if non is present yet.

int plm_demux_get_num_video_streams(plm_demux_t *self);


// Returns the number of audio streams found in the system header. This will
// attempt to read the system header if non is present yet.

int plm_demux_get_num_audio_streams(plm_demux_t *self);


// Rewind the internal buffer. See plm_buffer_rewind().

void plm_demux_rewind(plm_demux_t *self);


// Get whether the file has ended. This will be cleared on seeking or rewind.

int plm_demux_has_ended(plm_demux_t *self);


// Seek to a packet of the specified type with a PTS just before specified time.
// If force_intra is TRUE, only packets containing an intra frame will be
// considered - this only makes sense when the type is PLM_DEMUX_PACKET_VIDEO_1.
// Note that the specified time is considered 0-based, regardless of the first
// PTS in the data source.

plm_packet_t *plm_demux_seek(plm_demux_t *self, double time, int type, int force_intra);


// Get the PTS of the first packet of this type. Returns PLM_PACKET_INVALID_TS
// if not packet of this packet type can be found.

double plm_demux_get_start_time(plm_demux_t *self, int type);


// Get the duration for the specified packet type - i.e. the span between the
// the first PTS and the last PTS in the data source. This only makes sense when
// the underlying data source is a file or fixed memory.

double plm_demux_get_duration(plm_demux_t *self, int type);


// Decode and return the next packet. The returned packet_t is valid until
// the next call to plm_demux_decode() or until the demuxer is destroyed.

plm_packet_t *plm_demux_decode(plm_demux_t *self);



// -----------------------------------------------------------------------------
// plm_video public API
// Decode MPEG1 Video ("mpeg1") data into raw YCrCb frames


// Create a video decoder with a plm_buffer as source.

plm_video_t *plm_video_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);


// Destroy a video decoder and free all data.

void plm_video_destroy(plm_video_t *self);


// Get whether a sequence header was found and we can accurately report on
// dimensions and framerate.

int plm_video_has_header(plm_video_t *self);


// Get the framerate in frames per second.

double plm_video_get_framerate(plm_video_t *self);


// Get the display width/height.

int plm_video_get_width(plm_video_t *self);
int plm_video_get_height(plm_video_t *self);


// Set "no delay" mode. When enabled, the decoder assumes that the video does
// *not* contain any B-Frames. This is useful for reducing lag when streaming.
// The default is FALSE.

void plm_video_set_no_delay(plm_video_t *self, int no_delay);


// Get the current internal time in seconds.

double plm_video_get_time(plm_video_t *self);


// Set the current internal time in seconds. This is only useful when you
// manipulate the underlying video buffer and want to enforce a correct
// timestamps.

void plm_video_set_time(plm_video_t *self, double time);


// Rewind the internal buffer. See plm_buffer_rewind().

void plm_video_rewind(plm_video_t *self);


// Get whether the file has ended. This will be cleared on rewind.

int plm_video_has_ended(plm_video_t *self);


// Decode and return one frame of video and advance the internal time by
// 1/framerate seconds. The returned frame_t is valid until the next call of
// plm_video_decode() or until the video decoder is destroyed.

plm_frame_t *plm_video_decode(plm_video_t *self);


// Convert the YCrCb data of a frame into interleaved R G B data. The stride
// specifies the width in bytes of the destination buffer. I.e. the number of
// bytes from one line to the next. The stride must be at least
// (frame->width * bytes_per_pixel). The buffer pointed to by *dest must have a
// size of at least (stride * frame->height).
// Note that the alpha component of the dest buffer is always left untouched.

void plm_frame_to_rgb(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_bgr(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_rgba(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_bgra(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_argb(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_abgr(plm_frame_t *frame, uint8_t *dest, int stride);


// -----------------------------------------------------------------------------
// plm_audio public API
// Decode MPEG-1 Audio Layer II ("mp2") data into raw samples


// Create an audio decoder with a plm_buffer as source.

plm_audio_t *plm_audio_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done);


// Destroy an audio decoder and free all data.

void plm_audio_destroy(plm_audio_t *self);


// Get whether a frame header was found and we can accurately report on
// samplerate.

int plm_audio_has_header(plm_audio_t *self);


// Get the samplerate in samples per second.

int plm_audio_get_samplerate(plm_audio_t *self);


// Get the current internal time in seconds.

double plm_audio_get_time(plm_audio_t *self);


// Set the current internal time in seconds. This is only useful when you
// manipulate the underlying video buffer and want to enforce a correct
// timestamps.

void plm_audio_set_time(plm_audio_t *self, double time);


// Rewind the internal buffer. See plm_buffer_rewind().

void plm_audio_rewind(plm_audio_t *self);


// Get whether the file has ended. This will be cleared on rewind.

int plm_audio_has_ended(plm_audio_t *self);


// Decode and return one "frame" of audio and advance the internal time by
// (PLM_AUDIO_SAMPLES_PER_FRAME/samplerate) seconds. The returned samples_t
// is valid until the next call of plm_audio_decode() or until the audio
// decoder is destroyed.

plm_samples_t *plm_audio_decode(plm_audio_t *self);



#ifdef __cplusplus
}
#endif

#endif
