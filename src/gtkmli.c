#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "gtk-ml.h"

#define GTKMLI_VERSION "gtkmli ver. 0.0.0"

struct ParameterPrototype {
    gboolean exists;
    gboolean takes_value;
    gboolean multiple_allowed;
    gboolean takes_rest;
    char short_opt;
    const char *long_opt;
    const char *value_name;
    const char *description;
};

GTKML_PRIVATE const struct ParameterPrototype PARAMS[] = {
    ['r'] = { 1, 0, 0, 0, 'r', "run-application", NULL, "Run the result of :f or :e as a GTK application." },
    ['d'] = { 1, 0, 0, 0, 'd', "dump-result", NULL, "Dump the result of :f or :e." },
    ['D'] = { 1, 0, 0, 0, 'D', "dump-program", NULL, "Dump the bytecode of :f or :e."  },
    ['e'] = { 1, 1, 1, 0, 'e', "eval", "S-EXPR", "Evaluate an S-EXPR." },
    ['f'] = { 1, 1, 0, 0, 'f', "file", "PATH", "Load and execute a file from a PATH." },
    ['F'] = { 1, 1, 0, 0, 'F', "file-and-run", "PATH", "Load and execute a file from a PATH and then run it as a GTK application." },
    ['h'] = { 1, 0, 0, 0, 'h', "help", NULL, "Print this message and exit." },
    ['V'] = { 1, 0, 0, 0, 'V', "version", NULL, "Print the current version and exit." },
    ['v'] = { 1, 0, 0, 0, 'v', "verbose", NULL, "Print some additional information." },
    [255] = {0, 0, 0, 0, 0, NULL, NULL, NULL },
};

#define array_len(array) (sizeof(array) / sizeof(array[0]))

void print_help(const char *arg0) {
    fprintf(stderr, "usage: %s PARAMS\n\n", arg0);
    fprintf(stderr, "# PARAMS in alphabetical order\n");
    size_t desc_offset = 0;
    for (size_t i = 0; i < array_len(PARAMS); i++) {
        if (PARAMS[i].exists) {
            size_t offset = 0;
            offset += strlen("  ");
            if (PARAMS[i].short_opt) {
                offset += strlen(":_  ");
            }
            offset += strlen(PARAMS[i].long_opt) + 2;
            if (PARAMS[i].takes_value) {
                if (PARAMS[i].multiple_allowed) {
                    offset += strlen("...");
                }
                offset += strlen(PARAMS[i].value_name) + 4;
                if (PARAMS[i].takes_rest) {
                    offset += strlen("...");
                }
            }
            if (offset > desc_offset) {
                desc_offset = offset;
            }
        }
    }
    for (size_t i = 0; i < array_len(PARAMS); i++) {
        if (PARAMS[i].exists) {
            size_t offset = 0;
            offset += fprintf(stderr, "  ");
            if (PARAMS[i].short_opt) {
                offset += fprintf(stderr, ":%c  ", PARAMS[i].short_opt);
            }
            offset += fprintf(stderr, ":%s ", PARAMS[i].long_opt);
            if (PARAMS[i].takes_value) {
                if (PARAMS[i].multiple_allowed) {
                    offset += fprintf(stderr, "...");
                }
                offset += fprintf(stderr, "%s", PARAMS[i].value_name);
                if (PARAMS[i].takes_rest) {
                    offset += fprintf(stderr, "...");
                }
                offset += fprintf(stderr, "    ");
            }
            for (size_t j = 0; j < desc_offset - offset; j++) {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "%s\n", PARAMS[i].description);
        }
    }
}

void print_version(const char *arg0) {
    fprintf(stderr, "%s: %s", arg0, GTKMLI_VERSION);
    fprintf(stderr, "%s: %s", arg0, GTKML_VERSION);
}

int main(int argc, const char **argv) {
    GtkMl_S *err = NULL;

    GtkMl_Context *ctx = gtk_ml_new_context();

    const char *rest = NULL;

    GtkMl_HashTrie flags;
    gtk_ml_new_hash_trie(&flags, &GTKML_DEFAULT_HASHER);

    GtkMl_HashTrie opts;
    gtk_ml_new_hash_trie(&opts, &GTKML_DEFAULT_HASHER);

    GtkMl_S *s_flags = gtk_ml_new_map(ctx, NULL, NULL);
    GtkMl_S *s_opts = gtk_ml_new_map(ctx, NULL, NULL);

    for (size_t i = 0; i < array_len(PARAMS); i++) {
        if (PARAMS[i].exists) {
            if (PARAMS[i].takes_value) {
                const char *long_opt = PARAMS[i].long_opt;
                GtkMl_HashTrie new;
                gtk_ml_new_hash_trie(&new, &GTKML_DEFAULT_HASHER);
                GtkMl_S *default_value;
                if (PARAMS[i].multiple_allowed || PARAMS[i].takes_rest) {
                    default_value = gtk_ml_new_array(ctx, NULL);
                } else {
                    default_value = gtk_ml_new_nil(ctx, NULL);
                }
                if (PARAMS[i].takes_rest) {
                    rest = PARAMS[i].long_opt;
                }
                gtk_ml_hash_trie_insert(&new, &opts, gtk_ml_new_keyword(ctx, NULL, 0, long_opt, strlen(long_opt)), default_value);
                gtk_ml_del_hash_trie(ctx, &opts, gtk_ml_delete_void_reference);
                opts = new;
            } else {
                const char *long_opt = PARAMS[i].long_opt;
                GtkMl_HashTrie new;
                gtk_ml_new_hash_trie(&new, &GTKML_DEFAULT_HASHER);
                gtk_ml_hash_trie_insert(&new, &flags, gtk_ml_new_keyword(ctx, NULL, 0, long_opt, strlen(long_opt)), gtk_ml_new_false(ctx, NULL));
                gtk_ml_del_hash_trie(ctx, &flags, gtk_ml_delete_void_reference);
                flags = new;
            }
        }
    }

    for (int i = 1; i < argc; i++) {
        const char *long_opt = NULL;
        if (argv[i][0] == ':') {
            if (argv[i][2] == 0) {
                if (!PARAMS[(unsigned char) argv[i][1]].exists) {
                    err = gtk_ml_error(ctx, "param-error", "unrecognized short option", 0, 0, 0, 1, gtk_ml_new_keyword(ctx, NULL, 0, "param", strlen("param")), gtk_ml_new_string(ctx, NULL, argv[i], strlen(argv[i])));
                    gtk_ml_dumpf(ctx, stderr, NULL, err);
                    fprintf(stderr, "\n");
                    gtk_ml_del_context(ctx);
                    return 1;
                }
                long_opt = PARAMS[(unsigned char) argv[i][1]].long_opt;
            } else {
                long_opt = &argv[i][1];
            }
        } else {
            err = gtk_ml_error(ctx, "param-error", "parameters must be preceded by `:`", 0, 0, 0, 1, gtk_ml_new_keyword(ctx, NULL, 0, "param", strlen("param")), gtk_ml_new_string(ctx, NULL, argv[i], strlen(argv[i])));
            gtk_ml_dumpf(ctx, stderr, NULL, err);
            fprintf(stderr, "\n");
            gtk_ml_del_context(ctx);
            return 1;
        }

        GtkMl_S *keyword = gtk_ml_new_keyword(ctx, NULL, 0, long_opt, strlen(long_opt));
        if (rest && strcmp(rest, long_opt) == 0) {
            ++i;
            for (; i < argc; i++) {
                GtkMl_S *value = gtk_ml_new_string(ctx, NULL, argv[i], strlen(argv[i]));

                GtkMl_S *current = gtk_ml_hash_trie_get(&opts, keyword);
                if (current->kind == GTKML_S_ARRAY) {
                    GtkMl_Array new;
                    gtk_ml_array_trie_push(&new, &current->value.s_array.array, value);
                    gtk_ml_del_array_trie(ctx, &current->value.s_array.array, gtk_ml_delete_value_reference);
                    current->value.s_array.array = new;
                } else {
                    GtkMl_HashTrie new;
                    gtk_ml_hash_trie_insert(&new, &opts, keyword, value);
                    gtk_ml_del_hash_trie(ctx, &opts, gtk_ml_delete_void_reference);
                    opts = new;
                }
            }
        } else if (gtk_ml_hash_trie_contains(&flags, keyword)) {
            GtkMl_HashTrie new;
            gtk_ml_hash_trie_insert(&new, &flags, keyword, gtk_ml_new_true(ctx, NULL));
            gtk_ml_del_hash_trie(ctx, &flags, gtk_ml_delete_void_reference);
            flags = new;
        } else if (gtk_ml_hash_trie_contains(&opts, keyword)) {
            ++i;
            if (i == argc) {
                err = gtk_ml_error(ctx, "value-error", "missing value to parameter", 0, 0, 0, 1, gtk_ml_new_keyword(ctx, NULL, 0, "param", strlen("param")), keyword);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            GtkMl_S *value = gtk_ml_new_string(ctx, NULL, argv[i], strlen(argv[i]));

            GtkMl_S *current = gtk_ml_hash_trie_get(&opts, keyword);
            if (current->kind == GTKML_S_ARRAY) {
                GtkMl_Array new;
                gtk_ml_array_trie_push(&new, &current->value.s_array.array, value);
                gtk_ml_del_array_trie(ctx, &current->value.s_array.array, gtk_ml_delete_value_reference);
                current->value.s_array.array = new;
            } else {
                GtkMl_HashTrie new;
                gtk_ml_hash_trie_insert(&new, &opts, keyword, value);
                gtk_ml_del_hash_trie(ctx, &opts, gtk_ml_delete_void_reference);
                opts = new;
            }
        } else {
            err = gtk_ml_error(ctx, "param-error", "unrecognized parameter", 0, 0, 0, 1, gtk_ml_new_keyword(ctx, NULL, 0, "param", strlen("param")), keyword);
            gtk_ml_dumpf(ctx, stderr, NULL, err);
            fprintf(stderr, "\n");
            gtk_ml_del_context(ctx);
            return 1;
        }
    }

    s_flags->value.s_map.map = flags;
    s_opts->value.s_map.map = opts;
    gtk_ml_push(ctx, s_flags);
    gtk_ml_push(ctx, s_opts);

    GtkMl_S *help_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['h'].long_opt, strlen(PARAMS['h'].long_opt));
    GtkMl_S *help_opt = gtk_ml_hash_trie_get(&flags, help_kw);
    if (help_opt->kind == GTKML_S_TRUE) {
        print_help(argv[0]);
        return 0;
    }

    GtkMl_S *version_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['V'].long_opt, strlen(PARAMS['V'].long_opt));
    GtkMl_S *version_opt = gtk_ml_hash_trie_get(&flags, version_kw);
    if (version_opt->kind == GTKML_S_TRUE) {
        print_version(argv[0]);
        return 0;
    }

    GtkMl_S *verbose_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['v'].long_opt, strlen(PARAMS['v'].long_opt));
    GtkMl_S *verbose_opt = gtk_ml_hash_trie_get(&flags, verbose_kw);
    if (verbose_opt->kind == GTKML_S_TRUE) {
        GtkMl_S *s_flags = gtk_ml_new_map(ctx, NULL, NULL);
        s_flags->value.s_map.map = flags;
        GtkMl_S *s_opts = gtk_ml_new_map(ctx, NULL, NULL);
        s_opts->value.s_map.map = opts;

        fprintf(stderr, "running with flags: ");
        gtk_ml_dumpf(ctx, stderr, &err, s_flags);
        fprintf(stderr, "\nrunning with options: ");
        gtk_ml_dumpf(ctx, stderr, &err, s_opts);
        fprintf(stderr, "\n");
    }

    gboolean requires_file = 1;
    GtkMl_S *eval_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['e'].long_opt, strlen(PARAMS['e'].long_opt));
    GtkMl_S *eval_opt = gtk_ml_hash_trie_get(&opts, eval_kw);
    if (gtk_ml_array_trie_len(&eval_opt->value.s_array.array) > 0) {
        requires_file = 0;

        for (size_t i = 0; i < gtk_ml_array_trie_len(&eval_opt->value.s_array.array); i++) {
            GtkMl_S *eval = gtk_ml_array_trie_get(&eval_opt->value.s_array.array, i);
            char *src = gtk_ml_to_c_str(eval);

            GtkMl_S *lambda;
            if (!(lambda = gtk_ml_loads(ctx, &err, src))) {
                free(src);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            gtk_ml_push(ctx, lambda);

            GtkMl_Builder *builder = gtk_ml_new_builder(ctx);

            if (!gtk_ml_compile_program(ctx, builder, &err, lambda)) {
                free(src);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            GtkMl_Program *program = gtk_ml_build(ctx, &err, builder);
            if (!program) {
                free(src);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            gtk_ml_load_program(ctx, program);

            GtkMl_S *dump_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['D'].long_opt, strlen(PARAMS['D'].long_opt));
            GtkMl_S *dump_opt = gtk_ml_hash_trie_get(&flags, dump_kw);
            if (dump_opt->kind == GTKML_S_TRUE) {
                if (!gtk_ml_dumpf_program(ctx, stdout, &err)) {
                    free(src);
                    gtk_ml_dumpf(ctx, stderr, NULL, err);
                    fprintf(stderr, "\n");
                    gtk_ml_del_context(ctx);
                    return 1;
                }
            }

            GtkMl_S *start = gtk_ml_get_export(ctx, &err, program->start);
            if (!start) {
                free(src);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            if (!gtk_ml_run_program(ctx, &err, start, NULL)) {
                free(src);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }
        }
    }

    char *src = NULL;
    GtkMl_S *file_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['f'].long_opt, strlen(PARAMS['f'].long_opt));
    GtkMl_S *file_opt = gtk_ml_hash_trie_get(&opts, file_kw);

    GtkMl_S *file_and_run_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['F'].long_opt, strlen(PARAMS['F'].long_opt));
    GtkMl_S *file_and_run_opt = gtk_ml_hash_trie_get(&opts, file_and_run_kw);
    if (file_opt->kind == GTKML_S_NIL && file_and_run_opt->kind == GTKML_S_NIL) {
        if (requires_file) {
            err = gtk_ml_error(ctx, "missing-parameter-error", "a required parameter is missing", 0, 0, 0, 1, gtk_ml_new_keyword(ctx, NULL, 0, "param", strlen("param")), file_kw);
            gtk_ml_dumpf(ctx, stderr, NULL, err);
            fprintf(stderr, "\n");
            gtk_ml_del_context(ctx);
            return 1;
        }
    } else {
        char *file;
        if (file_and_run_opt->kind == GTKML_S_ARRAY) {
            file = gtk_ml_to_c_str(file_and_run_opt);
        } else {
            file = gtk_ml_to_c_str(file_opt);
        }
        gboolean is_binary = 0;
        size_t len = strlen(file);
        if (len >= strlen(".bgtkml")) {
            if (strcmp(file + len - strlen(".bgtkml"), ".bgtkml") == 0) {
                is_binary = 1;
            }
        }

        GtkMl_Program *program;
        if (is_binary) {
            GtkMl_Deserializer deserf;
            gtk_ml_new_deserializer(&deserf);
            FILE *bgtkml = fopen(file, "r");
            program = gtk_ml_deserf_program(&deserf, ctx, bgtkml, &err);
            if (!program) {
                free(file);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            fclose(bgtkml);

            gtk_ml_load_program(ctx, program);
        } else {
            GtkMl_S *lambda;
            if (!(lambda = gtk_ml_load(ctx, &src, &err, file))) {
                GtkMl_S *err_kw = gtk_ml_new_keyword(ctx, NULL, 0, "err", strlen("err"));
                GtkMl_S *err_kind = gtk_ml_hash_trie_get(&err->value.s_map.map, err_kw);

                if (gtk_ml_equal(err_kind, gtk_ml_new_symbol(ctx, NULL, 0, "io-error", strlen("io-error")))) {
                    int errnum = errno;

                    GtkMl_HashTrie new;

                    if (errnum == ENOENT) {
                        gtk_ml_hash_trie_insert(&new, &err->value.s_map.map, gtk_ml_new_keyword(ctx, NULL, 0, "filename", strlen("filename")), gtk_ml_new_string(ctx, NULL, file, strlen(file)));
                        gtk_ml_del_hash_trie(ctx, &err->value.s_map.map, gtk_ml_delete_void_reference);
                        err->value.s_map.map = new;
                    }

                    gtk_ml_hash_trie_insert(&new, &err->value.s_map.map, gtk_ml_new_keyword(ctx, NULL, 0, "errno", strlen("errno")), gtk_ml_new_int(ctx, NULL, errnum));
                    gtk_ml_del_hash_trie(ctx, &err->value.s_map.map, gtk_ml_delete_void_reference);
                    err->value.s_map.map = new;

                    const char *desc = strerror(errnum);
                    gtk_ml_hash_trie_insert(&new, &err->value.s_map.map, gtk_ml_new_keyword(ctx, NULL, 0, "desc", strlen("desc")), gtk_ml_new_string(ctx, NULL, desc, strlen(desc)));
                    gtk_ml_del_hash_trie(ctx, &err->value.s_map.map, gtk_ml_delete_void_reference);
                    err->value.s_map.map = new;
                }

                free(file);
                gtk_ml_dumpf(ctx, stderr, NULL, err);

                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            gtk_ml_push(ctx, lambda);

            GtkMl_Builder *builder = gtk_ml_new_builder(ctx);

            if (!gtk_ml_compile_program(ctx, builder, &err, lambda)) {
                if (src) {
                    free(src);
                }
                free(file);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            program = gtk_ml_build(ctx, &err, builder);
            if (!program) {
                if (src) {
                    free(src);
                }
                free(file);
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }

            gtk_ml_load_program(ctx, program);
        }
        free(file);

        GtkMl_S *dump_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['D'].long_opt, strlen(PARAMS['D'].long_opt));
        GtkMl_S *dump_opt = gtk_ml_hash_trie_get(&flags, dump_kw);
        if (dump_opt->kind == GTKML_S_TRUE) {
            if (!gtk_ml_dumpf_program(ctx, stdout, &err)) {
                if (src) {
                    free(src);
                }
                gtk_ml_dumpf(ctx, stderr, NULL, err);
                fprintf(stderr, "\n");
                gtk_ml_del_context(ctx);
                return 1;
            }
        }

        GtkMl_S *start = gtk_ml_get_export(ctx, &err, program->start);
        if (!start) {
            if (src) {
                free(src);
            }
            gtk_ml_dumpf(ctx, stderr, NULL, err);
            fprintf(stderr, "\n");
            gtk_ml_del_context(ctx);
            return 1;
        }

        if (!gtk_ml_run_program(ctx, &err, start, NULL)) {
            if (src) {
                free(src);
            }
            gtk_ml_dumpf(ctx, stderr, NULL, err);
            fprintf(stderr, "\n");
            gtk_ml_del_context(ctx);
            return 1;
        }
    }

    GtkMl_S *result = gtk_ml_peek(ctx);

    if (!result) {
        if (src) {
            free(src);
        }
        gtk_ml_dumpf(ctx, stderr, NULL, err);
        fprintf(stderr, "\n");
        gtk_ml_del_context(ctx);
        return 1;
    }

    GtkMl_S *dump_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['d'].long_opt, strlen(PARAMS['d'].long_opt));
    GtkMl_S *dump_opt = gtk_ml_hash_trie_get(&flags, dump_kw);
    if (dump_opt->kind == GTKML_S_TRUE) {
        if (!gtk_ml_dumpf(ctx, stdout, &err, result)) {
            if (src) {
                free(src);
            }
            gtk_ml_dumpf(ctx, stderr, NULL, err);
            fprintf(stderr, "\n");
            gtk_ml_del_context(ctx);
            return 1;
        }
        fprintf(stdout, "\n");
    }

    int status = 0;
    GtkMl_S *run_kw = gtk_ml_new_keyword(ctx, NULL, 0, PARAMS['r'].long_opt, strlen(PARAMS['r'].long_opt));
    GtkMl_S *run_opt = gtk_ml_hash_trie_get(&flags, run_kw);
    if (run_opt->kind == GTKML_S_TRUE || file_and_run_opt->kind == GTKML_S_ARRAY) {
        status = g_application_run(G_APPLICATION(result->value.s_userdata.userdata), 0, NULL);
    }

    gtk_ml_del_context(ctx);
    if (src) {
        free(src);
    }

    return status;
}
