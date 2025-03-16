#include "wasm-component.h"

int render_count = 0;
int count = 0;

void button_callback(void* args) {
    ASSERT(args == NULL);
    
    count++;
}

Element* render_component()
{
    render_count++;

    return element("div", children(
        text_element_with_text("h1", "Hello, world!"),
        text_element_with_text("h2", arena_sprintf(&r_arena, "Count: %d", count)),
        text_element_with_text("p", arena_sprintf(&r_arena, "Render count: %d", render_count)),
        button("Increment", button_callback, NULL)
    ));
}