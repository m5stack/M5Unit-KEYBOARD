/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Key matrix visualizer example using M5UnitUnified for UnitTab5Keyboard.

  Renders the 5 x 14 Tab5 keyboard matrix on the Tab5 LCD with per-key
  color-coded state:
    Green  — software repeat firing (isRepeating)
    Blue   — hold threshold just crossed this frame (wasHold)
    Cyan   — key held past threshold and still down (isHolding)
    White  — key pressed but not yet at hold threshold (isPressed)
    Black  — key released (outline only)

  The screen is redrawn every loop iteration so that transient one-frame
  states (isRepeating, wasHold) are always visible. Tab5 has sufficient
  CPU headroom to absorb the cost.
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedKEYBOARD.h>
#include <M5HAL.hpp>
#include <M5Utility.h>

// This example targets UnitTab5Keyboard (Normal mode) only.
// `M5UnitUnifiedKEYBOARD.h` always includes `unit_Tab5Keyboard.hpp`, so no
// build-time define is required to opt into the Tab5 unit here.

namespace {
auto& lcd = M5.Display;

m5::unit::UnitUnified Units;
m5::unit::UnitTab5Keyboard unit;

// Tab5 Keyboard connects via ExtPort1 (10-pin internal connector) on M5Stack Tab5.
// Pin assignment matches the SimpleDisplay example.
//   INT = GPIO50 (J9 pin 10) -- handled by config_t default (irq_pin = 50)
//   SDA = GPIO0  (J9 pin 7)
//   SCL = GPIO1  (J9 pin 8)
constexpr int8_t TAB5_KEYBOARD_SDA = 0;
constexpr int8_t TAB5_KEYBOARD_SCL = 1;

// Matrix geometry (5 rows x 14 cols) -- pulled from the unit's namespace constants
// so the code automatically tracks any future change in KEY_COL_COUNT / KEY_COUNT.
constexpr uint8_t MATRIX_ROWS = m5::unit::tab5_keyboard::KEY_COUNT / m5::unit::tab5_keyboard::KEY_COL_COUNT;
constexpr uint8_t MATRIX_COLS = m5::unit::tab5_keyboard::KEY_COL_COUNT;

// Visual style — cell fill colors ordered by priority (highest first).
constexpr uint16_t COLOR_BG            = TFT_BLACK;
constexpr uint16_t COLOR_CELL_BORDER   = TFT_DARKGRAY;
constexpr uint16_t COLOR_CELL_LABEL    = TFT_LIGHTGRAY;  // label on released cell
constexpr uint16_t COLOR_FILL_REPEAT   = TFT_GREEN;      // isRepeating (one-shot)
constexpr uint16_t COLOR_FILL_WAS_HOLD = TFT_BLUE;       // wasHold (one-shot)
constexpr uint16_t COLOR_FILL_HOLDING  = TFT_CYAN;       // isHolding (sustained)
constexpr uint16_t COLOR_FILL_PRESSED  = TFT_WHITE;      // isPressed (normal)

// Key state enum — used by draw_cell() to select fill / label colors.
enum class CellState : uint8_t {
    Released  = 0,
    Pressed   = 1,
    Holding   = 2,
    WasHold   = 3,
    Repeating = 4,
};
// Minimum gap between adjacent cells; keeps a visible grid even on small displays.
constexpr int CELL_PADDING = 2;

bool setup_tab5_keyboard()
{
    // Normal mode is required: this example reads bitwise per-key state, which is
    // only populated in Normal mode. INT pin defaults to GPIO50 (Tab5 ExtPort1).
    {
        auto cfg = unit.config();
        cfg.mode = m5::unit::tab5_keyboard::Mode::Normal;
        // Enable software auto-repeat so isRepeating() / wasHold() states are
        // exercised and rendered with distinct colors.
        cfg.software_repeat      = true;
        cfg.repeat_initial_ms    = 1000;
        cfg.repeat_rate_ms       = 1000;
        cfg.holding_threshold_ms = 500;
        unit.config(cfg);
    }

    M5_LOGI("Tab5 ExtPort1 I2C: SDA:%d SCL:%d", TAB5_KEYBOARD_SDA, TAB5_KEYBOARD_SCL);
    Wire.end();
    Wire.begin(TAB5_KEYBOARD_SDA, TAB5_KEYBOARD_SCL, unit.component_config().clock);
    if (!Units.add(unit, Wire) || !Units.begin()) {
        return false;
    }

    // begin() applies cfg.mode and (when start_periodic is true) enables the matching INT
    // and starts draining events, so no manual writeInterruptEnable()/startPeriodicMeasurement().
    M5.Log.printf("Firmware:%02X\n", unit.firmwareVersion());
    return true;
}

// Pre-computed cell rectangles. Filled once by layout_matrix() in setup().
struct CellRect {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
};
CellRect cells[MATRIX_ROWS][MATRIX_COLS];

// Maximum bytes for a single cell label including the null terminator.
constexpr size_t LABEL_BUF_LEN = 6;

const char* modifier_label(const uint8_t row, const uint8_t col)
{
    // Tab5 keyboard modifier key positions (see unit_Tab5Keyboard.hpp isModifierKey()).
    if (row == 3 && col == 0) return "Sym";
    if (row == 3 && col == 1) return "Aa";
    if (row == 4 && col == 0) return "Ctrl";
    if (row == 4 && col == 1) return "Alt";
    return nullptr;
}

const char* control_label(const char c)
{
    switch (c) {
        case 0x08:
            return "BS";
        case 0x09:
            return "Tab";
        case 0x0A:
        case 0x0D:
            return "Ret";
        case 0x1B:
            return "ESC";
        case 0x20:
            return "Sp";
        case 0x7F:
            return "DEL";
        default:
            return nullptr;
    }
}

// Map USB HID keycodes that hidUsageToChar() does not translate to ASCII
// (Escape, Forward Delete, arrow keys, etc.) to short labels. Detection goes
// through the HID keycode because the ASCII path returns 0 for these keys.
const char* hid_named_label(const uint8_t hid_keycode)
{
    switch (hid_keycode) {
        case 0x29:
            return "ESC";  // Escape
        case 0x4C:
            return "DEL";  // Forward Delete
        case 0x4F:
            return "R";  // Right Arrow
        case 0x50:
            return "L";  // Left Arrow
        case 0x51:
            return "D";  // Down Arrow
        case 0x52:
            return "U";  // Up Arrow
        default:
            return nullptr;
    }
}

// Compute the live label for (row, col). Reflects the current Sym/Aa state via
// keyMatrixToChar(), so pressing Sym swaps the rendered glyphs to their symbol
// variants, and Aa adds the shift modifier.
void cell_label(const uint8_t row, const uint8_t col, char* dst, const size_t n)
{
    const char* mod = modifier_label(row, col);
    if (mod != nullptr) {
        snprintf(dst, n, "%s", mod);
        return;
    }

    // Arrow keys are detected via HID keycode (they have no ASCII representation).
    // Mirror keyMatrixToChar()'s Sym dispatch so the Sym layer's arrows are picked up too.
    const auto mapping = unit.isSym() ? m5::unit::tab5_keyboard::keyMatrixToHidSym(row, col)
                                      : m5::unit::tab5_keyboard::keyMatrixToHidBase(row, col);
    const char* named  = hid_named_label(mapping.keycode);
    if (named != nullptr) {
        snprintf(dst, n, "%s", named);
        return;
    }

    const char c     = unit.keyMatrixToChar(row, col);
    const char* ctrl = control_label(c);
    if (ctrl != nullptr) {
        snprintf(dst, n, "%s", ctrl);
    } else if (c >= 0x21 && c < 0x7F) {
        dst[0] = c;
        dst[1] = '\0';
    } else if (c != 0) {
        snprintf(dst, n, "0x%02X", static_cast<uint8_t>(c));
    } else {
        dst[0] = '\0';
    }
}

void layout_matrix()
{
    const int cw    = lcd.width() / MATRIX_COLS;
    const int ch    = lcd.height() / MATRIX_ROWS;
    const int off_x = (lcd.width() - cw * MATRIX_COLS) / 2;
    const int off_y = (lcd.height() - ch * MATRIX_ROWS) / 2;
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            cells[row][col] = CellRect{static_cast<int16_t>(off_x + col * cw), static_cast<int16_t>(off_y + row * ch),
                                       static_cast<int16_t>(cw), static_cast<int16_t>(ch)};
        }
    }
}

// Return the fill color for a given cell state.
uint16_t cell_fill_color(const CellState state)
{
    switch (state) {
        case CellState::Repeating:
            return COLOR_FILL_REPEAT;
        case CellState::WasHold:
            return COLOR_FILL_WAS_HOLD;
        case CellState::Holding:
            return COLOR_FILL_HOLDING;
        case CellState::Pressed:
            return COLOR_FILL_PRESSED;
        default:
            return COLOR_BG;
    }
}

// Choose a legible text color given the cell background.
// Dark backgrounds (black / blue / green) → white text.
// Light backgrounds (white / cyan)        → black text.
uint16_t cell_label_color(const CellState state)
{
    switch (state) {
        case CellState::Holding:
        case CellState::Pressed:
            return TFT_BLACK;
        default:
            return COLOR_CELL_LABEL;  // white / gray on dark background
    }
}

// Render one cell directly to the LCD. Caller is expected to wrap multiple
// draw_cell() calls in lcd.startWrite() / lcd.endWrite() to batch the SPI
// transactions.
void draw_cell(const uint8_t row, const uint8_t col, const CellState state)
{
    const CellRect& r = cells[row][col];
    const int16_t ix  = r.x + CELL_PADDING;
    const int16_t iy  = r.y + CELL_PADDING;
    const int16_t iw  = r.w - CELL_PADDING * 2;
    const int16_t ih  = r.h - CELL_PADDING * 2;
    if (iw <= 0 || ih <= 0) {
        return;
    }

    const uint16_t fill = cell_fill_color(state);
    lcd.fillRect(ix, iy, iw, ih, fill);
    lcd.drawRect(ix, iy, iw, ih, COLOR_CELL_BORDER);

    const uint16_t fg = cell_label_color(state);
    const uint16_t bg = fill;

    // Cell coordinate label "row,col" at top-left for easier debugging.
    if (iw >= 24 && ih >= 16) {
        lcd.setTextDatum(top_left);
        lcd.setTextColor(fg, bg);
        lcd.setCursor(ix + 2, iy + 2);
        lcd.printf("%u,%u", row, col);
    }

    // Key imprint centered in the cell. Computed live so Sym/Aa modifier state
    // is reflected (e.g. pressing Sym swaps glyphs to their symbol variants).
    char label[LABEL_BUF_LEN];
    cell_label(row, col, label, sizeof(label));
    if (label[0] != '\0' && iw >= 20 && ih >= 24) {
        lcd.setTextDatum(middle_center);
        lcd.setTextColor(fg, bg);
        lcd.drawString(label, ix + iw / 2, iy + ih / 2 + 4);
    }
}

// Determine the highest-priority visual state for the key at flat index kidx.
// Priority (highest → lowest): Repeating > WasHold > Holding > Pressed > Released.
CellState key_cell_state(const uint8_t kidx)
{
    if (unit.isRepeating(kidx)) {
        return CellState::Repeating;
    }
    if (unit.wasHold(kidx)) {
        return CellState::WasHold;
    }
    if (unit.isHolding(kidx)) {
        return CellState::Holding;
    }
    if (unit.isPressed(kidx)) {
        return CellState::Pressed;
    }
    return CellState::Released;
}

// Per-cell cached state for incremental redraw. Initialized to Released so the
// first frame after setup() repaints every cell.
CellState prev_states[m5::unit::tab5_keyboard::KEY_COUNT]{};
bool prev_sym          = false;
bool prev_aa           = false;
bool initial_draw_done = false;

// Redraw only the cells whose state changed since the previous frame. Sym/Aa
// changes relabel every cell, so they force a full redraw.
void draw_dirty_cells()
{
    const bool sym_changed = (unit.isSym() != prev_sym);
    const bool aa_changed  = (unit.isAa() != prev_aa);
    const bool full_redraw = !initial_draw_done || sym_changed || aa_changed;

    lcd.startWrite();  // Batch SPI transactions across all cell draws this frame.
    for (uint8_t row = 0; row < MATRIX_ROWS; ++row) {
        for (uint8_t col = 0; col < MATRIX_COLS; ++col) {
            const uint8_t kidx    = static_cast<uint8_t>(row * MATRIX_COLS + col);
            const CellState state = key_cell_state(kidx);
            if (!full_redraw && state == prev_states[kidx]) {
                continue;
            }
            draw_cell(row, col, state);
            prev_states[kidx] = state;
        }
    }
    lcd.endWrite();

    prev_sym          = unit.isSym();
    prev_aa           = unit.isAa();
    initial_draw_done = true;
}

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);

    // Keep the LCD in landscape (Tab5: 1280x720 native).
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(3);
    }
    lcd.fillScreen(TFT_LIGHTGRAY);

    if (!setup_tab5_keyboard()) {
        M5_LOGE("Failed to begin UnitTab5Keyboard");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }
    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    // Direct-to-LCD rendering: paint the matrix background once, then let
    // draw_dirty_cells() incrementally refresh only the cells whose state
    // changed. This avoids the ~450 KB / frame cost of pushing a full-screen
    // sprite over SPI.
    lcd.setFont(&fonts::AsciiFont8x16);
    lcd.fillScreen(COLOR_BG);

    layout_matrix();
    draw_dirty_cells();  // initial_draw_done=false → forces a full repaint.
}

void loop()
{
    M5.update();
    Units.update();

    draw_dirty_cells();
    m5::utility::delay(1000 / 60);
}
