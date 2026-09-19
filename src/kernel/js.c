#include "kernel.h"
#include "klib.h"
#include <stdlib.h>
#include "quickjs.h"

extern const unsigned char ts_bundle[];
extern const unsigned int ts_bundle_len;

// host functions (typeos)

static JSValue t_print(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	uint8_t attr = vga_get_attr();

	if (argc > 1 && JS_IsString(argv[1])) {
		const char* cn = JS_ToCString(ctx, argv[1]);
		if (cn) {
			attr = vga_color_attr(cn, 0x07);
			JS_FreeCString(ctx, cn);
		}
	}
	if (argc > 0) {
		const char* s = JS_ToCString(ctx, argv[0]);
		if (s) {
			vga_write_attr(s, attr);
			JS_FreeCString(ctx, s);
		}
	}
	return JS_UNDEFINED;
}

static JSValue t_clear(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	vga_clear();
	return JS_UNDEFINED;
}

static JSValue t_set_cursor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	int32_t x = 0, y = 0;
	if (argc > 0)
		JS_ToInt32(ctx, &x, argv[0]);
	if (argc > 1)
		JS_ToInt32(ctx, &y, argv[1]);
	vga_set_cursor(x, y);
	return JS_UNDEFINED;
}

static JSValue t_get_cursor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	int x, y;
	vga_get_cursor(&x, &y);
	JSValue arr = JS_NewArray(ctx);
	JS_SetPropertyUint32(ctx, arr, 0, JS_NewInt32(ctx, x));
	JS_SetPropertyUint32(ctx, arr, 1, JS_NewInt32(ctx, y));
	return arr;
}

static JSValue t_readline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	char* line = kbd_readline(); // blocks until Enter
	JSValue v = JS_NewString(ctx, line);
	free(line);
	return v;
}

static JSValue t_width(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	return JS_NewInt32(ctx, VGA_W);
}

static JSValue t_height(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
	return JS_NewInt32(ctx, VGA_H);
}

// error reporting

static void js_dump_error(JSContext* ctx) {
	JSValue exc = JS_GetException(ctx);
	const char* msg = JS_ToCString(ctx, exc);
	kprintf("typeos script error: %s\n", msg ? msg : "(unknown error)");
	if (msg)
		JS_FreeCString(ctx, msg);

	JSValue stack = JS_GetPropertyStr(ctx, exc, "stack");
	if (JS_IsString(stack)) {
		const char* st = JS_ToCString(ctx, stack);
		if (st) {
			serial_puts(st);
			vga_write_attr(st, 0x07);
			JS_FreeCString(ctx, st);
		}
	}
	JS_FreeValue(ctx, stack);
	JS_FreeValue(ctx, exc);
}

// runtime setup

void js_run(void) {
	JSRuntime* rt = JS_NewRuntime();
	if (!rt)
		panic("js: could not create runtime");
	JSContext* ctx = JS_NewContext(rt);
	if (!ctx)
		panic("js: could not create context");

	JSValue global = JS_GetGlobalObject(ctx);
	JSValue typeos_obj = JS_NewObject(ctx);

	static const struct {
		const char* name;
		JSCFunction* fn;
		int nargs;
	} host_fns[] = {
		{"print", t_print, 2},			{"clear", t_clear, 0},		 {"setCursor", t_set_cursor, 2},
		{"getCursor", t_get_cursor, 0}, {"readline", t_readline, 0}, {"width", t_width, 0},
		{"height", t_height, 0},
	};

	for (size_t i = 0; i < sizeof(host_fns) / sizeof(host_fns[0]); i++)
		JS_SetPropertyStr(ctx, typeos_obj, host_fns[i].name,
						  JS_NewCFunction(ctx, host_fns[i].fn, host_fns[i].name, host_fns[i].nargs));

	JS_SetPropertyStr(ctx, typeos_obj, "version", JS_NewString(ctx, "0.0.1"));
	JS_SetPropertyStr(ctx, global, "typeos", typeos_obj);
	JS_FreeValue(ctx, global);

	JSValue ret = JS_Eval(ctx, (const char*)ts_bundle, ts_bundle_len, "<typeos>", JS_EVAL_TYPE_GLOBAL);
	if (JS_IsException(ret)) {
		js_dump_error(ctx);
		panic("typeos script crashed");
	}
	JS_FreeValue(ctx, ret);

	// the shell never returns; if it does, cleanly free everything

	JS_FreeContext(ctx);
	JS_FreeRuntime(rt);
}