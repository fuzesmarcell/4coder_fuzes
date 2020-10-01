// TOP

#if !defined(FCODER_DEFAULT_BINDINGS_CPP)
#define FCODER_DEFAULT_BINDINGS_CPP

#include "4coder_default_include.cpp"

// NOTE(allen): Users can declare their own managed IDs here.

#if !defined(META_PASS)
#include "generated/managed_id_metadata.cpp"
#endif

/*
 TODO

- Jump to symbol which is under your cursor
- type macro function highlight when we have table look up for code index
- Function annotaion when typing a function
- Rectangle delete
- History to go back and forth in a buffer like in visual studio
- Jump from one brace to the top brace and vica versa

DONE
- Brace scop highlight
- Custom type highlight
*/

CUSTOM_COMMAND_SIG(fuzes_open_matching_file_in_same_view_cpp)
CUSTOM_DOC("If the current file is a *.cpp or *.h, attempts to open the corresponding *.h or *.cpp file in the same view.")
{
    View_ID view = get_active_view(app, Access_Always);
    Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
    Buffer_ID new_buffer = 0;
    if (get_cpp_matching_file(app, buffer, &new_buffer)){
        view_set_buffer(app, view, new_buffer, 0);
        view_set_active(app, view);
    }
}

function void
fuzes_highlight_functions_and_macros(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id)
{
    i64 keyword_length = 0;
    i64 start_pos = 0;
    i64 end_pos = 0;
    
    Token_Array array = get_token_array_from_buffer(app, buffer);
    if (array.tokens != 0)
    {
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        i64 first_index = token_index_from_pos(&array, visible_range.first);
        Token_Iterator_Array it = token_iterator_index(0, &array, first_index);
        
        for(;;){
            Token *token = token_it_read(&it);
            if (token->pos >= visible_range.one_past_last){
                break;
            }
            
            if (keyword_length != 0 && token->kind == TokenBaseKind_ParentheticalOpen &&
                token->sub_kind == TokenCppKind_ParenOp){
                end_pos = token->pos;
            }
            
            if (token->kind == TokenBaseKind_Identifier) {
                if (keyword_length == 0) {
                    start_pos = token->pos;
                }
                
                keyword_length += 1;
            } else {
                keyword_length = 0;
            }
            
            if (start_pos != 0 && end_pos != 0) {
                Range_i64 range = { 0 };
                range.start = start_pos;
                range.end = end_pos;
                
                paint_text_color(app, text_layout_id, range, 0xFFC55F46);
                
                end_pos = 0;
                start_pos = 0;
            }
            
            if(!token_it_inc_all(&it)) {
                break;
            }
        }
    }
}

function void
fuzes_highlight_types(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id)
{
    Scratch_Block scratch(app);
    FColor col = {0};
    
    Token_Array array = get_token_array_from_buffer(app, buffer);
    if (array.tokens != 0) {
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        i64 first_index = token_index_from_pos(&array, visible_range.first);
        Token_Iterator_Array it = token_iterator_index(0, &array, first_index);
        
        for(;;){
            Token *token = token_it_read(&it);
            
            if (token->pos >= visible_range.one_past_last) {
                break;
            }
            
            if (token->kind == TokenBaseKind_Identifier &&
                token->sub_kind == TokenCppKind_Identifier) {
                
                String_Const_u8 token_as_string = push_token_lexeme(app, scratch, buffer, token);
                
                b32 FoundSymbol = false;
                for(Buffer_ID buf = get_buffer_next(app, 0, Access_Always);
                    buf != 0 && !FoundSymbol;
                    buf = get_buffer_next(app, buf, Access_Always))
                {
                    Code_Index_File *file = code_index_get_file(buf);
                    if(file != 0){
                        for(i32 i = 0; i < file->note_array.count; i++){
                            Code_Index_Note *note = file->note_array.ptrs[i];
                            
                            if(note->note_kind == CodeIndexNote_Type) {
                                if(string_match(note->text, token_as_string, StringMatch_Exact)){
                                    Range_i64 range = { 0 };
                                    range.start = token->pos;
                                    range.end = token->pos + token->size;
                                    
                                    paint_text_color(app, text_layout_id, range, 0xFF0C73A6);
                                }
                            }
                            else if(note->note_kind == CodeIndexNote_Macro)
                            {
                                if(string_match(note->text, token_as_string, StringMatch_Exact)){
                                    Range_i64 range = { 0 };
                                    range.start = token->pos;
                                    range.end = token->pos + token->size;
                                    
                                    paint_text_color(app, text_layout_id, range, 0xFF4AABA7);
                                }
                            }
                            
                            // NOTE(fuzesm): When this is activated it is just to slow.
                            // Will only work if they are hashed...
#if 0                            
                            else if(note->note_kind == CodeIndexNote_Function)
                            {
                                if(string_match(note->text, token_as_string, StringMatch_Exact)){
                                    Range_i64 range = { 0 };
                                    range.start = token->pos;
                                    range.end = token->pos + token->size;
                                    
                                    paint_text_color(app, text_layout_id, range, 0xFFA44633);
                                }
                            }
#endif
                            
                        }
                    }
                }
            }
            
            if (!token_it_inc_all(&it)) {
                break;
            }
        }
    }
}

function void
fuzes_draw_scope_highlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                           i64 pos, ARGB_Color *colors, i32 color_count){
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
        Token *token = token_it_read(&it);
        if (token != 0 && token->kind == TokenBaseKind_ScopeOpen){
            pos = token->pos + token->size;
        }
        else{
            if (token_it_dec_all(&it)){
                token = token_it_read(&it);
                if (token->kind == TokenBaseKind_ScopeClose &&
                    pos == token->pos + token->size){
                    pos = token->pos;
                }
            }
        }
    }
    
    draw_enclosures(app, text_layout_id, buffer,
                    pos, FindNest_Scope, RangeHighlightKind_CharacterHighlight,
                    0, 0, colors, color_count);
}

function void
fuzes_draw_file_bar(Application_Links *app, View_ID view_id, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar){
    Scratch_Block scratch(app);
    
    draw_rectangle_fcolor(app, bar, 0.f, fcolor_id(defcolor_bar));
    
    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    
    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));
    
    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                     Line_Ending_Kind);
    switch (*eol_setting){
        case LineEndingKind_Binary:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
        }break;
        
        case LineEndingKind_LF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
        }break;
        
        case LineEndingKind_CRLF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
        }break;
    }
    
    if(global_keyboard_macro_is_recording){
        push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" macro rec"));
    }
    
    u8 space[3];
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
        String_u8 str = Su8(space, 0, 3);
        if (dirty != 0){
            string_append(&str, string_u8_litexpr(" "));
        }
        if (HasFlag(dirty, DirtyState_UnsavedChanges)){
            string_append(&str, string_u8_litexpr("*"));
        }
        if (HasFlag(dirty, DirtyState_UnloadedChanges)){
            string_append(&str, string_u8_litexpr("!"));
        }
        push_fancy_string(scratch, &list, pop2_color, str.string);
    }
    
    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

function void
fuzes_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id,
                    Buffer_ID buffer, Text_Layout_ID text_layout_id,
                    Rect_f32 rect){
    ProfileScope(app, "render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = metrics.normal_advance*global_config.cursor_roundness;
    f32 mark_thickness = (f32)global_config.mark_thickness;
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        draw_cpp_token_colors(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
                {string_u8_litexpr("HACK"), 0xFFFF00FF},
                {string_u8_litexpr("IMPORTANT"), 0xFFFFFF00},
            };
            draw_comment_highlights(app, buffer, text_layout_id,
                                    &token_array, pairs, ArrayCount(pairs));
        }
    }
    else{
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);
    
    // NOTE(allen): Scope highlight
    if (global_config.use_scope_highlight){
        //Color_Array colors = finalize_color_array(defcolor_back_cycle);
        ARGB_Color color = fcolor_resolve(fcolor_argb(0.16f, 0.9f, 0.9f, 1.0f));
        fuzes_draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, &color, 1);
    }
    
    if (global_config.use_error_highlight || global_config.use_jump_highlight){
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (global_config.use_error_highlight){
            draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                                 fcolor_id(defcolor_highlight_junk));
        }
        
        // NOTE(allen): Search highlight
        if (global_config.use_jump_highlight){
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer){
                draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                                     fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // NOTE(allen): Color parens
    if (global_config.use_paren_helper){
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    fuzes_highlight_functions_and_macros(app, buffer, text_layout_id);
    fuzes_highlight_types(app, buffer, text_layout_id);
    
    // NOTE(allen): Line highlight
    if (global_config.highlight_line_at_cursor && is_active_view){
        i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number,
                            fcolor_id(defcolor_highlight_cursor_line));
    }
    
    // NOTE(allen): Whitespace highlight
    b64 show_whitespace = false;
    view_get_setting(app, view_id, ViewSetting_ShowWhitespace, &show_whitespace);
    if (show_whitespace){
        if (token_array.tokens == 0){
            draw_whitespace_highlight(app, buffer, text_layout_id, cursor_roundness);
        }
        else{
            draw_whitespace_highlight(app, text_layout_id, &token_array, cursor_roundness);
        }
    }
    
    // NOTE(allen): Cursor
    switch (fcoder_mode){
        case FCoderMode_Original:
        {
            draw_original_4coder_style_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
        }break;
        case FCoderMode_NotepadLike:
        {
            draw_notepad_style_cursor_highlight(app, view_id, buffer, text_layout_id, cursor_roundness);
        }break;
    }
    
    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer);
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    draw_set_clip(app, prev_clip);
}

function void
fuzes_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
    ProfileScope(app, "fuzes render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    Rect_f32 region = draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        fuzes_draw_file_bar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt * 1.125f, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)){
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): query bars
    region = default_draw_query_bars(app, region, view_id, face_id);
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    // NOTE(allen): draw the buffer
    fuzes_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

void
custom_layer_init(Application_Links *app){
    Thread_Context *tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    default_framework_init(app);
    
    // NOTE(allen): default hooks and command maps
    set_all_default_hooks(app);
    mapping_init(tctx, &framework_mapping);
    
    MappingScope();
    SelectMapping(&framework_mapping);
    SelectMap(mapid_code);
    
    Bind(jump_to_definition, KeyCode_J, KeyCode_Control);
    Bind(fuzes_open_matching_file_in_same_view_cpp, KeyCode_3, KeyCode_Alt);
    
#if OS_MAC
    setup_mac_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
#else
    setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
#endif
    
    set_custom_hook(app, HookID_RenderCaller, fuzes_render_caller);
    
    suppress_mouse(app);
}

#endif //FCODER_DEFAULT_BINDINGS

// BOTTOM