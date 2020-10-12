
#if defined (__STDC__) && !defined(__STDC_VERSION__)
	#define inline
#endif

inline unsigned vmstrlen(const char *str)
{
	unsigned l = 0;
	while (*str != '\0') {
		str++; l++;
	}
	return l;
}

inline char *vmstrcpy(char *dest, const char *src, unsigned *length)
{
	unsigned l = 0;
	char *odest = dest;
	while (*src != '\0') {
		*dest = *src;
		src++; dest++; l++;
	}
	*dest = '\0';
	if (length) *length = l;
	return odest;
}

/* utoa (unsigned to string) */
static char *vmutoa(unsigned value, char *dest, const int base, unsigned *length)
{
	/* special case! */
	if (value == 0) {
		*dest = '0';
		*(dest + 1) = '\0';
		if (length) *length = 1;
		return dest;
	}

	char *odest = dest;
	/* Compute digits (units first)
	 * and append to string as ascii */
	unsigned digit, l = 0;
	while (value != 0) {
		digit = value % base;
		if (digit > 9) *dest = (digit - 9) + 'a'; /* hex */
		else *dest = digit + '0';
		value /= base;
		dest++; l++;
	}
	*dest = '\0';

	/* reverse so units is on the right */
	char temp;
	char *start = odest, *end = dest - 1;
	while (start < end) {
		temp = *start;
		*start = *end;
		*end = temp;
		start++; end--;
	}

	if (length) *length = l;
	return odest;
}

#define STRBUF_SIZE (128)
static char vmsprintf_buf[STRBUF_SIZE];

/* Format string.
 * Has %u, %s, %x, width, and 0-padding.
 * Other than strings, all args are pointers to the
 * actual value (as no guarantee that sizeof(void*)>=sizeof(int)) */
int vmsprintf(char *dest, const char *format, const void *const *args)
{
	unsigned arglen, width, nchars = 0;
	char fc, pad_char;

	while ((fc = *format++) != '\0') {
		if (fc == '%') {
			width = 0; pad_char = ' ';

			if (fc == '0') {
				pad_char = '0';
				fc = *format++;
			}

			while (fc >= '0' && fc <= '9') {
				width = 10 * width + (fc - '0');
				fc = *format++;
			}

			/* format specifiers */
			switch (fc)
			{
				case '%': /* escapes itself */
					vmsprintf_buf[0] = '%';
					vmsprintf_buf[1] = '\0';
					arglen = 1;
					break;

				case 'u':
					vmutoa(*(unsigned *)(*args), vmsprintf_buf, 10, &arglen);
					args++;
					break;

				case 'x':
					vmutoa(*(unsigned *)(*args), vmsprintf_buf, 16, &arglen);
					args++;
					break;

				case 's':
					if (vmstrlen(*args) >= STRBUF_SIZE)
						return -1;
					vmstrcpy(vmsprintf_buf, *args, &arglen);
					args++;
					break;

				default: return -1; /* specifier missing */
			}

			if (arglen < width) {
				unsigned num_to_pad = width - arglen;
				nchars += num_to_pad;
				while (num_to_pad > 0) {
					*dest = pad_char;
					dest++; num_to_pad--;
				}
			}

			vmstrcpy(dest, vmsprintf_buf, 0);
			dest += arglen;
			nchars += arglen;
		} else {
			/* no formatting */
			*dest = fc;
			dest++; nchars++;
		}
	}
	*dest = '\0';

	return (int)nchars;
}
