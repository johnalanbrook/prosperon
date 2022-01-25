#pragma once

#include <stdint.h>
#include <sys/types.h>

/**
 * Arrow types.
 */
typedef enum {
	BMOL_ARR_NONE = 0,
	BMOL_ARR_RIGHT,
	BMOL_ARR_LEFT,
	BMOL_ARR_DOWN,
	BMOL_ARR_UP
} bmol_arr_type;

/**
 * Defines arrow.
 */
typedef struct {
	uint8_t type:3;     ///< Arrow type.
	uint8_t inner:1;    ///< Associated path is inner path.
	uint8_t seen:1;     ///< Has been seen.
	uint8_t visited:1;  ///< Has been visited.
} bmol_arrow;

/**
 * Defines path segment.
 */
typedef struct {
	uint8_t type; ///< Arrow type.
	int dx;       ///< Path segment width.
	int dy;       ///< Path segment height.
} bmol_path_seg;

/**
 * Defines outliner object.
 */
typedef struct {
	int width;               ///< Bitmap width.
	int height;              ///< Bitmap height.
	uint8_t const* data;     ///< Bitmap data.
	bmol_path_seg* segments; ///< Path segment buffer.
	int segments_size;       ///< Path segment buffer length.
	int segments_cap;        ///< Path segment buffer capacity.
	bmol_arrow arrow_grid[]; ///< Grid arrows.
} bmol_outliner;

/**
 * Allocate outliner object.
 *
 * @param width The bitmap width.
 * @param height The bitmap height.
 * @param data The bitmap data.
 * @return Outliner object on success.
 */
extern bmol_outliner* bmol_alloc(int width, int height, uint8_t const* data);

/**
 * Free outliner object.
 *
 * @param outliner The outline object.
 */
extern void bmol_free(bmol_outliner* outliner);

/**
 * Find paths in bitmap data.
 *
 * @param outliner The outliner object.
 * @param out_size The number of path fragments.
 * @return The path fragments.
 */
extern bmol_path_seg const* bmol_find_paths(bmol_outliner* outliner, int* out_size);

/**
 * Calculate the SVG path length.
 *
 * @return The length of the SVG path string.
 */
extern size_t bmol_svg_path_len(bmol_outliner* outliner);

/**
 * Create SVG path from segments.
 *
 * @param outliner The outline object.
 * @param buffer The buffer to write the SVG path into.
 * @param buf_size The buffer capacity.
 * @return The length of the generated SVG path.
 */
extern size_t bmol_svg_path(bmol_outliner* outliner, char buffer[], size_t buf_size);

/**
 * Set bitmap data. Must have the same dimensions as the current one.
 *
 * @param outliner The outline robject.
 * @param data The new bitmap data.
 */
extern void bmol_set_bitmap(bmol_outliner* outliner, uint8_t const* data);
