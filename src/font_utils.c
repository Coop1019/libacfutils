/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license in the file COPYING
 * or http://www.opensource.org/licenses/CDDL-1.0.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file COPYING.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2022 Saso Kiselkov. All rights reserved.
 */
/*
 * mt_cairo_render is a multi-threaded cairo rendering surface with
 * built-in double-buffering and OpenGL compositing. You only need to
 * provide a callback that renders into the surface using a passed
 * cairo_t and then call mt_cairo_render_draw at regular intervals to
 * display the rendered result.
 */

#include <XPLMGraphics.h>

#include "acfutils/core.h"
#include "acfutils/font_utils.h"
#include "acfutils/helpers.h"

/*
 * This weird macro construct is needed to implement freetype error code
 * to string translation. It defines a static ft_errors table that we can
 * traverse to translate an error code into a string.
 */
#undef	FTERRORS_H_
#define	FT_ERRORDEF(e, v, s)	{ e, s },
#define	FT_ERROR_START_LIST	{
#define	FT_ERROR_END_LIST	{ 0, NULL } };
static const struct {
	int		err_code;
	const char	*err_msg;
} ft_errors[] =
#include FT_ERRORS_H

const char *
ft_err2str(FT_Error err)
{
	for (int i = 0; ft_errors[i].err_msg != NULL; i++)
		if (ft_errors[i].err_code == err)
			return (ft_errors[i].err_msg);
	return (NULL);
}

/*
 * Simple font loading front-end.
 *
 * @param fontdir A path to the directory from which to load the font.
 * @param fontfile A font file name. This is concatenated onto the fontdir
 *	with a path separator. If you only want to provide one string with
 *	a full path to the font file, pass that in fontdir and set
 *	fontfile = NULL.
 * @param ft FreeType library handle.
 * @param font Return FreeType font face object pointer. Release this after
 *	the cairo font face object using FT_DoneFace.
 * @param cr_font Return cairo font face object pointer. Release this before
 *	the freetype font face using cairo_font_face_destroy.
 *
 * Return B_TRUE if loading the font was successfull, B_FALSE otherwise. In
 *	case of error, the reason is logged using logMsg.
 */
bool_t
try_load_font(const char *fontdir, const char *fontfile, FT_Library ft,
    FT_Face *font, cairo_font_face_t **cr_font)
{
	char *fontpath = mkpathname(fontdir, fontfile, NULL);
	FT_Error err;

	if ((err = FT_New_Face(ft, fontpath, 0, font)) != 0) {
		logMsg("Error loading font file %s: %s", fontpath,
		    ft_err2str(err));
		free(fontpath);
		return (B_FALSE);
	}

	*cr_font = cairo_ft_font_face_create_for_ft_face(*font, 0);

	free(fontpath);

	return (B_TRUE);
}
