#include "hex.h"

#include <string.h>
#include <stdlib.h>

/***
 * turn a char into its hex value
 * A '5' becomes a 5, 'B' (or 'b') becomes 11.
 * @param c the input
 * @returns the value
 */
int32_t unhex(char c)
{
    if ( c >= '0' && c <= '9' )
        return(c - '0');
    else if ( c >= 'a' && c <= 'f' )
        return(c - 'a' + 10);
    else if ( c >= 'A' && c <= 'F' )
        return(c - 'A' + 10);
    return(-1);
}

/***
 * @brief Check n characters of a string to see if it can be interpreted as a hex string
 * @param str the input
 * @param n the size of input to check, 0 to check until null terminator found
 * @returns 0 on error, 1 if okay, strlen(str) if okay and n == 0
 */
int32_t is_hexstr(const char *str, int32_t n)
{
    if ( str == 0 || str[0] == 0 )
        return 0;

    int32_t i;
    for (i=0; str[i]!=0; i++)
    {
        if ( n > 0 && i >= n )
            break;
        if ( unhex(str[i]) < 0 )
            break;
    }
    if ( n == 0 )
        return i;
    return i == n;
}

/***
 * Decode a 2 character hex string into its value
 * @param hex the string (i.e. 'ff')
 * @returns the value (i.e. 255)
 */
unsigned char _decode_hex(const char *hex) 
{ 
    return (unhex(hex[0])<<4) | unhex(hex[1]); 
}

/***
 * @brief Turn a hex string into bytes
 * NOTE: If there is an odd number of characters in a null-terminated str, treat the first char as a "0"
 * 
 * @param[out] bytes where to store the output (will be cleared if hex has invalid chars)
 * @param[in] n number of bytes to process
 * @param[in] in the input string (will ignore CR/LF)
 * @returns the number of bytes processed (i.e. strlen(in)/2)
 */
int32_t decode_hex(uint8_t *bytes, int32_t n,const char *in)
{
    // copy the input to manipulate it
    size_t len = strlen(in);
    char *str = (char*) malloc(len + 2); // space for terminator and adding a leading zero if necessary
    strcpy(str, in);
    // concentrate only on characters we will process
    if (len > n * 2)
        str[n * 2] = 0;
    // check for cr/lf
    char *pos = strchr(str, '\r');
    if (pos != NULL)
        pos[0] = 0;
    pos = strchr(str, '\n');
    if (pos != NULL)
        pos[0] = 0;
    // check for odd number of characters
    len = strlen(str);
    if (len % 2 == 1)
    {
        // append a zero to the front
        for(size_t i = len + 1; i > 0; --i)
            str[i] = str[i-1];
        str[0] = '0';
        len++;
    }

    // check validity of input
    if ( is_hexstr(str,len) <= 0 )
    {
        memset(bytes,0,n); // give no results
        free(str);
        return 0;
    }
    if ( n > 0 )
    {
        for (int i=0; i<n; i++)
        {
            if (str[i*2] == 0) // null is next
                break;
            bytes[i] = _decode_hex(&str[i*2]);
        }
    }
    free(str);
    return n;
}

/***
 * Convert a byte into its stringified representation
 * eg: 1 becomes '1', 11 becomes 'b'
 * @param c the byte
 * @returns the character
 */
char hexbyte(int32_t c)
{
    c &= 0xf;
    if ( c < 10 )
        return('0'+c);
    else if ( c < 16 )
        return('a'+c-10);
    else return(0);
}

/****
 * Convert a binary array of bytes into a hex string
 * @param hexbytes the result
 * @param message the array of bytes
 * @param len the length of message
 * @returns the length of hexbytes (including null terminator)
 */
int32_t init_hexbytes_noT(char *hexbytes, unsigned char *message, long len)
{
    if ( len <= 0 ) // parameter validation
    {
        hexbytes[0] = 0;
        return(1);
    }

    for (int i=0; i<len; i++)
    {
        hexbytes[i*2] = hexbyte((message[i]>>4) & 0xf);
        hexbytes[i*2 + 1] = hexbyte(message[i] & 0xf);
    }
    hexbytes[len*2] = 0;
    return((int32_t)len*2+1);
}

/***
 * Convert a bits256 into a hex character string
 * @param hexstr the results
 * @param x the input
 * @returns a pointer to hexstr
 */
char *bits256_str(char hexstr[65],bits256 x)
{
    init_hexbytes_noT(hexstr,x.bytes,sizeof(x));
    return(hexstr);
}
