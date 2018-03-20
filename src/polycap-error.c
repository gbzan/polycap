#include "config.h"
#include "polycap-error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* polycap_strdup_vprintf(const char *format, va_list args) {
	char *rv = NULL;

	if (vasprintf(&rv, format, args) < 0) {
		return NULL;
	}
	return rv;
}

polycap_error* polycap_error_new_valist(enum polycap_error_code code, const char *format, va_list args) {
	polycap_error *error;

	if (format == NULL) {
		fprintf(stderr, "polycap_error_new_valist: format cannot be NULL!\n");
		return NULL;
	}

	error = malloc(sizeof(polycap_error));
	error->code = code;
	error->message = polycap_strdup_vprintf(format, args);

	return error;
}

polycap_error* polycap_error_new(enum polycap_error_code code, const char *format, ...) {
	polycap_error* error;
	
	if (format == NULL) {
		fprintf(stderr, "polycap_error_new: format cannot be NULL!\n");
		return NULL;
	}

	va_list args;

	va_start(args, format);
	error = polycap_error_new_valist(code, format, args);
	va_end(args);

	return error;
}

polycap_error* polycap_error_new_literal(enum polycap_error_code code, const char *message) {
	polycap_error *error = NULL;

	if (message == NULL) {
		fprintf(stderr, "polycap_error_new_literal: message cannot be NULL!\n");
		return NULL;
	}

	error = malloc(sizeof(polycap_error));
	error->code = code;
	error->message = strdup(message);

	return error;
}

void polycap_error_free(polycap_error *error) {
	if (error == NULL)
		return;

	if (error->message)
		free(error->message);

	free(error);
}

polycap_error* polycap_error_copy(const polycap_error *error) {
	polycap_error *copy = NULL;

	if (error == NULL)
		return NULL;

	copy = malloc(sizeof(polycap_error));

	*copy = *error;

	copy->message = NULL;
	if (error->message)
		copy->message = strdup(error->message);

	return copy;
}

bool polycap_error_matches(const polycap_error *error, enum polycap_error_code code) {
	return error && error->code == code;
}

#define ERROR_OVERWRITTEN_WARNING "polycap_error set over the top of a previous polycap_error or uninitialized memory.\n" \
               "This indicates a bug in someone's code. You must ensure an error is NULL before it's set.\n" \
"The overwriting error message was: %s"

void polycap_set_error(polycap_error **err, enum polycap_error_code code , const char *format, ...) {
	polycap_error *new = NULL;

	va_list args;

	if (err == NULL)
		return;

	va_start(args, format);
	new = polycap_error_new_valist(code, format, args);
	va_end(args);

	if (*err == NULL)
		*err = new;
	else {
		fprintf(stderr, ERROR_OVERWRITTEN_WARNING, new->message);
		polycap_error_free(new);
	}
}

void polycap_set_error_literal(polycap_error **err, enum polycap_error_code code, const char *message) {
	if (err == NULL)
		return;

	if (*err == NULL)
		*err = polycap_error_new_literal(code, message);
	else
		fprintf(stderr, ERROR_OVERWRITTEN_WARNING, message);
}

void polycap_propagate_error(polycap_error **dest, polycap_error *src) {
	if (src == NULL) {
		fprintf(stderr, "polycap_propagate_error: src cannot be NULL");
		return;
	}

	if (dest == NULL) {
		if (src)
			polycap_error_free(src);
		return;
	}
	else {
		if (*dest != NULL) {
			fprintf(stderr, ERROR_OVERWRITTEN_WARNING, src->message);
			polycap_error_free(src);
		}
		else {
			*dest = src;
		}
	}
}

void polycap_clear_error(polycap_error **err) {
	if (err && *err) {
		polycap_error_free(*err);
		*err = NULL;
	}
}
