#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

typedef enum {
    RECIPE_LOADED,
    RECIPE_TITLE,
    RECIPE_IMAGE,
    RECIPE_SUBTITLE,
    RECIPE_DESCRIPTION,
    RECIPE_STEP,
    RECIPE_INFO,
} recipe_state;

static bool is_ws(char c) {
    return isspace(c);
}

static bool is_right_bracket(char c) {
    return c == ']';
}

static void trim_right(char* str, bool (*pred)(char)) {
    int len = strlen(str);
    for (int i = len - 1; i >= 0; --i) {
        if (pred(str[i])) {
            str[i] = '\0';
        } else {
            break;
        }
    }
}

static char* trim_left(char* str, bool (*pred)(char)) {
    char* result = str;
    while (pred(*result)) {
        ++result;
    }
    return result;
}

static char* trim(char* str) {
    trim_right(str, &is_ws);
    return trim_left(str, &is_ws);
}

static char* file_ext(const char* path) {
    return strrchr(path, '.');
}

static void emit_html_start(FILE* html) {
    fputs("<!DOCTYPE html><html>", html);
}

static void emit_head(FILE* html, const char* title) {
    fputs("<head>", html);
    fprintf(html, "<title>%s</title>", title);
    fputs("<link rel=\"stylesheet\" type=\"text/css\" href=\"css/style.css\">", html);
    fputs("<link rel=\"stylesheet\" type=\"text/css\" href=\"css/colors.css\">", html);
    fputs("</head>", html);
    fputs("<body>", html);
}

static void emit_body_title(FILE* html, const char* title) {
    fprintf(html, "<div><h1>%s</h1></div>", title);
}
static void emit_body_img(FILE* html, const char* path) {
    fprintf(html, "<div><img src=\"%s\"></div>", path);
}

static void emit_body_subtitle(FILE* html, const char* subtitle) {
    fprintf(html, "<div><blockquote>%s</blockquote></div>", subtitle);
}

static void emit_body_description(FILE* html, const char* info) {
    fprintf(html, "<div><p>%s</p></div>", info);
}

static void emit_body_step(FILE* html, const char* text, bool step_started) {
    if (!step_started) {
        fputs("<div class=\"step\"><ul>", html);
    }
    fprintf(html, "<li>%s</li>", text);
}

static void emit_body_info(FILE* html, const char* text, int color_index) {
    fputs("</ul>", html);
    fprintf(html, "<div class=\"info c%i\">%s</div>", color_index, text);
    fputs("</div>", html);
}

static void emit_html_end(FILE* html) {
    fputs("</body></html>", html);
}

bool emit_recipe(FILE* recipe, FILE* html) {
    recipe_state state = RECIPE_LOADED;

    bool step_started = false;
    int color_index = 0;

    emit_html_start(html);

    char linebuf[2048];
    while (fgets(linebuf, 2048, recipe) != NULL) {
        char* line = trim(linebuf);
        if (strlen(line) == 0) {
            continue;
        }
        //printf("Line: %s\n", line);
        char* rest_line = trim(&line[1]);
        switch (line[0]) {
            case '#': {
                // title
                if (state != RECIPE_LOADED) {
                    return false;
                }
                state = RECIPE_TITLE;
                emit_head(html, rest_line);
                emit_body_title(html, rest_line);
                break;
            }
            case '>': {
                // subtitle or info
                if (state == RECIPE_TITLE || state == RECIPE_IMAGE) {
                    state = RECIPE_SUBTITLE;
                    emit_body_subtitle(html, rest_line);
                } else if (state == RECIPE_STEP && step_started) {
                    state = RECIPE_INFO;
                    emit_body_info(html, rest_line, color_index);
                    color_index = (color_index + 1) % 4;
                    step_started = false;
                } else {
                    return false;
                }
                break;
            }
            case '[': {
                if (state != RECIPE_TITLE) {
                    return false;
                }
                state = RECIPE_IMAGE;
                trim_right(rest_line, &is_right_bracket);
                emit_body_img(html, rest_line);
                break;
            }
            case '*': {
                // step
                if (state == RECIPE_LOADED) {
                    return false;
                }
                state = RECIPE_STEP;
                emit_body_step(html, rest_line, step_started);
                step_started = true;
                break;
            }
            default: {
                // description
                if (state != RECIPE_TITLE &&
                    state != RECIPE_SUBTITLE &&
                    state != RECIPE_DESCRIPTION) {
                    return false;
                }
                state = RECIPE_DESCRIPTION;
                emit_body_description(html, line);
                break;
            }
        }
    }

    if (step_started) {
        fputs("</ul></div>", html);
        step_started = false;
    }

    emit_html_end(html);

    return true;
}

static bool convert_files(const char* src_path, const char* dst_path) {
    bool result = false;
    FILE* recipe = fopen(src_path, "r");
    if (recipe != NULL) {
        FILE* html = fopen(dst_path, "w");
        if (html != NULL) {
            if (emit_recipe(recipe, html)) {
                result = true;
            }
            fclose(html);
        }
        fclose(recipe);
    }
    return result;
}

int main(int argc, char* argv[]) {
    const char* src_dir = "cookbook/";
    const char* src_ext = ".md";
    size_t src_dir_len = strlen(src_dir);

    const char* dst_dir = "docs/";
    const char* dst_ext = ".html";
    size_t dst_dir_len = strlen(dst_dir);
    size_t dst_ext_len = strlen(dst_ext);

    DIR* dir = opendir(src_dir);
    if (dir != NULL) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            char* file_name = entry->d_name;
            size_t file_name_len = strlen(file_name);
            char* file_name_ext = file_ext(file_name);

            if (file_name > 0 &&
                src_dir_len + file_name_len < 1024 &&
                dst_dir_len + dst_ext_len + file_name_len < 1024 &&
                file_name_ext != NULL && strcmp(file_name_ext, src_ext) == 0) {

                char src_path[1024] = { 0 };
                strcpy(src_path, src_dir);
                strcpy(&src_path[src_dir_len], file_name);

                char dst_path[1024] = { 0 };
                strcpy(dst_path, dst_dir);
                strcpy(&dst_path[dst_dir_len], file_name);
                char* dst_path_ext = strstr(dst_path, src_ext);
                memcpy(dst_path_ext, dst_ext, sizeof(char) * dst_ext_len);

                printf("Converting %s to %s...\n", src_path, dst_path);

                if (!convert_files(src_path, dst_path)) {
                    puts("-> Conversion failed!");
                }
            }
        }
        closedir(dir);
    }

    return 0;
}
