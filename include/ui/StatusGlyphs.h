#pragma once

#include <stdint.h>

#include "ui/glyphs/bao.h"
#include "ui/glyphs/bing.h"
#include "ui/glyphs/chang.h"
#include "ui/glyphs/ching1.h"
#include "ui/glyphs/ching2.h"
#include "ui/glyphs/deng.h"
#include "ui/glyphs/ji.h"
#include "ui/glyphs/jian.h"
#include "ui/glyphs/jie.h"
#include "ui/glyphs/kang.h"
#include "ui/glyphs/sheng.h"
#include "ui/glyphs/shi.h"
#include "ui/glyphs/shin.h"
#include "ui/glyphs/tai.h"
#include "ui/glyphs/zheng.h"
#include "ui/glyphs/zhuang.h"

namespace Ui {
namespace StatusGlyphs {

struct Glyph {
    const uint8_t* bitmap;
    uint8_t width;
    uint8_t height;
};

// cht2bitmap stores each row MSB first, with zero bits as visible strokes.
constexpr Glyph Zhuang = {bitmap_49, 15, 16};  // 狀
constexpr Glyph Tai = {bitmap_37, 15, 16};     // 態
constexpr Glyph Bao = {bitmap_44, 15, 16};     // 飽
constexpr Glyph Shi = {bitmap_15, 15, 16};     // 食
constexpr Glyph Xin = {bitmap_43, 15, 16};     // 心
constexpr Glyph Qing = {bitmap_50, 15, 16};    // 情
constexpr Glyph QingClean = {bitmap_58, 15, 16};  // 清
constexpr Glyph Jie = {bitmap_56, 14, 16};     // 潔
constexpr Glyph Deng = {bitmap_60, 15, 16};    // 等
constexpr Glyph Ji = {bitmap_82, 15, 16};      // 級
constexpr Glyph Jian = {bitmap_35, 15, 16};    // 健
constexpr Glyph Kang = {bitmap_84, 15, 16};    // 康
constexpr Glyph Zheng = {bitmap_20, 15, 16};   // 正
constexpr Glyph Chang = {bitmap_97, 15, 16};   // 常
constexpr Glyph Sheng = {bitmap_59, 15, 16};   // 生
constexpr Glyph Bing = {bitmap_34, 15, 16};    // 病

}  // namespace StatusGlyphs
}  // namespace Ui
