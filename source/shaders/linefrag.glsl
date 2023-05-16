#version 330
out vec4 color;

in float dist;
in vec4 fcolor;

in float seg_len;
float pat = 0.5;

int pp = 0x0C24;

void main()
{
    color = fcolor;
    if (mod(dist,seg_len)/seg_len < 0.5)
      discard;
/*    
    int d = int(dist);
    
    if (pp * mod((d / 	20), 16) == 0)
      discard;
      
*/
/*      
    
    float patrn = 16 * mod(dist,seg_len)/seg_len;
    
    if (patrn < 8)
      discard;
 */
}
