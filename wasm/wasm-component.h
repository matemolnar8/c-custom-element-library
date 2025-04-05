#ifndef WASM_COMPONENT_H
#define WASM_COMPONENT_H

#include <stddef.h>
#include <stdarg.h>
#include "macros.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define WRITE_BUFFER_CAPACITY 4096
char write_buffer[WRITE_BUFFER_CAPACITY];
void platform_write(void *buffer, size_t len);
int printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int n = stbsp_vsnprintf(write_buffer, WRITE_BUFFER_CAPACITY, fmt, args);
    va_end(args);
    platform_write(write_buffer, n);
    return n;
}

#define ARENA_IMPLEMENTATION
#define ARENA_NOSTDIO
#define ARENA_ASSERT(cond) (!(cond) ? printf("%s:%d: %s: Assertion `%s' failed.", __FILE__, __LINE__, __func__, #cond), __builtin_trap() : 0)
#define ARENA_BACKEND ARENA_BACKEND_WASM_HEAPBASE
#include "arena.h"

#define ASSERT(cond) ARENA_ASSERT(cond)

typedef struct Element Element;

typedef struct {
    size_t count;
    size_t capacity;
    Element** items;
} Children;

typedef struct {
    const char* name;
    const char* value;
} Attribute;

typedef struct {
    size_t count;
    size_t capacity;
    Attribute** items;
} Attributes;

struct Element {
    const char* type;
    char* text;
    Children* children;
    void (*on_click)(void*);
    void* on_click_args;
    Attributes* attributes;
    size_t index;
};

typedef struct {
    size_t count;
    size_t capacity;
    Element** items;
} Elements;

#define struct_wrapper(type, ...) (type) __VA_ARGS__
#define _init_struct(type) \
    void *_init_##type(type value) { \
        type *result = arena_alloc(&r_arena, sizeof(type)); \
        *result = value; \
        return result; \
    } 

void platform_rerender();

Arena r_arena = {0};
Elements r_elements = {0};
#define ELEMENT_INDEX_OFFSET 69

_init_struct(Element);
_init_struct(Attribute);
_init_struct(Children);
_init_struct(Attributes);

#define ELEMENT(...) _init_Element(struct_wrapper(Element, __VA_ARGS__))
#define ATTRIBUTE(...) _init_Attribute(struct_wrapper(Attribute, __VA_ARGS__))
#define CHILDREN(...) _init_Children(struct_wrapper(Children, __VA_ARGS__))
#define ATTRIBUTES(...) _init_Attributes(struct_wrapper(Attributes, __VA_ARGS__))

// Adapted from arena.h using stb_sprintf
char *arena_sprintf(Arena* a, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int n = stbsp_vsnprintf(NULL, 0, format, args);
    va_end(args);

    ARENA_ASSERT(n >= 0);
    char *result = (char*)arena_alloc(a, n + 1);
    va_start(args, format);
    stbsp_vsnprintf(result, n + 1, format, args);
    va_end(args);

    return result;
}

#define children(...) _children(_NARG(__VA_ARGS__), __VA_ARGS__)
#define children_empty() _children(0)

Children* _children(size_t count, ...) {
    Children* result = CHILDREN({
        .count = 0,
        .capacity = 0,
        .items = NULL
    });

    va_list args;
    va_start(args, count);
    for (size_t i = 0; i < count; i++) {
        Element* element = va_arg(args, Element*);
        arena_da_append(&r_arena, result, element);
    }
    va_end(args);

    return result;
}

#define add_children(parent, ...) _add_children(parent, _NARG(__VA_ARGS__), __VA_ARGS__)
void _add_children(Element* parent, size_t count, ...) {
    va_list args;
    va_start(args, count);
    for (size_t i = 0; i < count; i++) {
        Element* element = va_arg(args, Element*);
        arena_da_append(&r_arena, parent->children, element);
    }
    va_end(args);
};

#define attributes(element, ...) _attributes(element, _NARG(__VA_ARGS__), __VA_ARGS__)
Element* _attributes(Element* element, size_t count, ...) {
    ASSERT(count % 2 == 0);

    element->attributes = ATTRIBUTES({
        .count = 0,
        .capacity = 0,
        .items = NULL
    });

    va_list args;
    va_start(args, count);
    for (size_t i = 0; i < count / 2; i++) {
        const char* name = va_arg(args, const char*);
        const char* value = va_arg(args, const char*);
        
        Attribute* attribute = ATTRIBUTE({
            .name = arena_strdup(&r_arena, name),
            .value = arena_strdup(&r_arena, value)
        });
        arena_da_append(&r_arena, element->attributes, attribute);
    }
    va_end(args);

    return element;
}

Element* element(const char* type, Children* children)
{
    Element* result = ELEMENT({
        .type = arena_strdup(&r_arena, type),
        .text = NULL,
        .children = children,
        .on_click = NULL,
        .on_click_args = NULL,
        .attributes = NULL,
        .index = 0
    });

    result->index = r_elements.count + ELEMENT_INDEX_OFFSET;
    arena_da_append(&r_arena, &r_elements, result);

    return result;
}

Element* button(char* text, void (*callback)(void*), void* args)
{
    Element* result = element("button", children_empty());

    result->text = text;
    result->on_click = callback;
    result->on_click_args = args;

    return result;
}

#define TEXT_CAPACITY 256

Element* text_element_empty(const char* type)
{
    Element* result = element(type, NULL);

    result->text = arena_alloc(&r_arena, TEXT_CAPACITY);

    return result;
}

Element* text_element(const char* type, const char* text)
{
    Element* result = element(type, NULL);

    result->text = arena_strdup(&r_arena, text);

    return result;
}

Element* render_component();

[[clang::export_name("render_component")]]
Element* render_component_internal() {
    arena_reset(&r_arena);
    r_elements.count = 0;
    r_elements.capacity = 0;

    return render_component();
}

[[clang::export_name("invoke_on_click")]]
void invoke_on_click(size_t element_index) {
    ASSERT(element_index - ELEMENT_INDEX_OFFSET < r_elements.count);
    Element* element = r_elements.items[element_index - ELEMENT_INDEX_OFFSET];

    if (element->on_click) {
        element->on_click(element->on_click_args);
        platform_rerender();
    }
}

#endif // WASM_COMPONENT_H