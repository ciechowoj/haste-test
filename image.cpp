#include <haste/image>
#include <haste/unittest>
#include <png.h>

namespace haste {

ndarray load_image(string path) {
  static const size_t bytes_to_check = 8;

  FILE *file = fopen(path.data(), "rb");

  if (!file) {
    throw runtime_error("Cannot open '", path, "'.");
  }

  png_byte buffer[bytes_to_check];

  if (fread(buffer, 1, bytes_to_check, file) != bytes_to_check) {
    fclose(file);
    throw runtime_error("Cannot read '", path, "'.");
  }

  if (png_sig_cmp(buffer, 0, bytes_to_check) != 0)
  {
    fclose(file);
    throw runtime_error("'", path, "' is not a png.");
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  if (png_ptr == nullptr) {
    fclose(file);
    throw runtime_error("Cannot create png_struct (", path, ").");
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if (info_ptr == nullptr) {
    png_destroy_read_struct(&png_ptr, nullptr, nullptr);
    fclose(file);
    throw runtime_error("Cannot create png_info (", path, ").");
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    fclose(file);
    throw runtime_error("libpng error (", path, ").");
  }

  png_init_io(png_ptr, file);
  png_set_sig_bytes(png_ptr, bytes_to_check);

  png_read_info(png_ptr, info_ptr);

  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  int color_type = png_get_color_type(png_ptr, info_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
  }

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  }

  if (bit_depth == 16) {
    png_set_strip_16(png_ptr);
  }

  if (color_type & PNG_COLOR_MASK_ALPHA) {
    png_set_strip_alpha(png_ptr);
  }

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }

  double gamma = 1.0;

  if (png_get_gAMA(png_ptr, info_ptr, &gamma)) {
    png_set_gamma(png_ptr, 1.0, gamma);
  }
  else {
    png_set_gamma(png_ptr, 1.0, 1.0);
  }

  auto result = ndarray::empty(ndshape(height, width, 3), ndtype::u8);
  auto stride = result.strides()[0];

  png_bytep* row_pointers = static_cast<png_bytep*>(::malloc(height * sizeof(png_bytep)));

  for (int i = 0; i < height; ++i) {
    row_pointers[i] = static_cast<png_bytep>(result.data()) + stride * i;
  }

  png_read_image(png_ptr, row_pointers);

  ::free(row_pointers);

  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
  fclose(file);

  return result;
}

unittest() {
  auto image = load_image("2x2.png");
  assert_eq(ilist(2, 2, 3), image.shape());
}

}
