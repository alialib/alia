// struct TextFragmentHandle
// {
//     void* ptr;
// };

// // Result of splitting a fragment at a width
// struct TextFragmentSplitResult
// {
//     TextFragmentHandle first_part; // The part that fits
//     float first_width; // Width of the first part
//     bool ready; // Whether the result is ready (true) or placeholder (false)
// } alia_text_fragment_split_result;

// // Font metrics associated with a fragment
// struct FontMetrics
// {
//     float ascent; // positive value
//     float descent; // positive value
//     float line_gap; // positive value

//     // float underline_position;
//     // float underline_thickness;

//     // float strikethrough_position;
//     // float strikethrough_thickness;
// };

// struct TextEngine
// {
//     void* engine_data;

//     // non-fragment queries

//     FontMetrics (*query_font_metrics)(
//         void* engine_data, alia_text_style style);

//         // --- core lifecycle ---

//         // Prepare a text fragment for use in the UI.
//         // Returns a handle to the fragment, which is essentially a pointer
//         to
//         // whatever internal data the engine has associated with the
//         fragment.
//         // Text fragments are immutable.
//         alia_text_fragment_handle (*prepare_text_fragment)(
//             void* engine_data, alia_text_style style, alia_utf8_string
//             text);

//     void (*destroy_text_fragment)(
//         void* engine_data, alia_text_fragment_handle handle);

//     // --- Layout queries ---

//     alia_text_size (*query_text_size)(
//         void* engine_data,
//         alia_text_fragment_handle handle,
//         float max_width,
//         bool* out_ready);

//     alia_text_fragment_split_result (*split_fragment_at_width)(
//         void* engine_data,
//         alia_text_fragment_handle handle,
//         float available_width);

//     // --- Drawing ---
//     void (*draw_text_fragment)(
//         void* engine_data,
//         alia_text_fragment_handle handle,
//         alia_vec2 baseline_position,
//         alia_color color,
//         struct alia_draw_context* ctx);

//     // Hit testing: screen position → character index
//     typedef size_t (*alia_hit_test_text_fn)(
//         void* engine_data, alia_text_fragment_handle handle, alia_vec2
//         point);

//     // Caret positioning: character index → screen position
//     typedef alia_vec2 (*alia_get_caret_position_fn)(
//         void* engine_data,
//         alia_text_fragment_handle handle,
//         size_t char_index);

//     // Selection bounds: text range → bounding box
//     typedef alia_rect (*alia_get_selection_bounds_fn)(
//         void* engine_data,
//         alia_text_fragment_handle handle,
//         size_t start_char_index,
//         size_t end_char_index);

//     // Pass notifications
//     void (*begin_pass)(void* engine_data, alia_pass_type pass_type);

// } alia_text_engine;
