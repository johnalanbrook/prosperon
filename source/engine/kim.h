#ifndef KIM_H
#define KIM_H

int utf8_bytes(char *s);
int utf8_count(char *s);
int decode_utf8(char **s);
void encode_utf8(char **s, int code);
void encode_kim(char **s, int code);
int decode_kim(char **s);
void utf8_to_kim(char **utf, char **kim);
void kim_to_utf8(char **kim, char **utf, int runes);

#endif
