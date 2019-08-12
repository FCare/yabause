#ifndef VDP1_PROG_COMPUTE_H
#define VDP1_PROG_COMPUTE_H

#include "ygl.h"

#define QuoteIdent(ident) #ident
#define Stringify(macro) QuoteIdent(macro)

#define POLYGON 0
#define DISTORTED 1
#define NORMAL 2

#define NB_COARSE_RAST_X 8
#define NB_COARSE_RAST_Y 8

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 8

//#define SHOW_QUAD

static const char vdp1_clear_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly highp uniform image2D outSurface;\n"
"void main()\n"
"{\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  imageStore(outSurface,texel,vec4(0.0));\n"
"}\n";

static const char vdp1_start_f[] =
SHADER_VERSION_COMPUTE
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"

"struct cmdparameter_struct{ \n"
"  float G[16];\n"
"  uint priority;\n"
"  uint w;\n"
"  uint h;\n"
"  uint flip;\n"
"  uint cor;\n"
"  uint cob;\n"
"  uint cog;\n"
"  uint type;\n"
"  uint CMDCTRL;\n"
"  uint CMDLINK;\n"
"  uint CMDPMOD;\n"
"  uint CMDCOLR;\n"
"  uint CMDSRCA;\n"
"  uint CMDSIZE;\n"
"  int CMDXA;\n"
"  int CMDYA;\n"
"  int CMDXB;\n"
"  int CMDYB;\n"
"  int CMDXC;\n"
"  int CMDYC;\n"
"  int CMDXD;\n"
"  int CMDYD;\n"
"  int P[8];\n"
"  uint CMDGRDA;\n"
"  uint SPCTL;\n"
"};\n"

"layout(local_size_x = "Stringify(LOCAL_SIZE_X)", local_size_y = "Stringify(LOCAL_SIZE_Y)") in;\n"
"layout(rgba8, binding = 0) writeonly highp uniform image2D outSurface;\n"
"layout(std430, binding = 1) readonly buffer VDP1RAM { uint Vdp1Ram[]; };\n"
"layout(std430, binding = 2) readonly buffer NB_CMD { uint nbCmd[]; };\n"
"layout(std430, binding = 3) readonly buffer CMD { \n"
"  cmdparameter_struct cmd[];\n"
"};\n"
"layout(location = 4) uniform vec2 upscale;\n"
// from here http://geomalgorithms.com/a03-_inclusion.html
// a Point is defined by its coordinates {int x, y;}
//===================================================================
// isLeft(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2  on the line
//            <0 for P2  right of the line
//    See: Algorithm 1 "Area of Triangles and Polygons"
"float isLeft( vec2 P0, vec2 P1, vec2 P2 ){\n"
//This can be used to detect an exact edge
"    return ( (P1.x - P0.x) * (P2.y - P0.y) - (P2.x -  P0.x) * (P1.y - P0.y) );\n"
"}\n"
// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only when P is outside)
"uint wn_PnPoly( vec2 P, vec2 V[5]){\n"
"  uint wn = 0;\n"    // the  winding number counter
   // loop through all edges of the polygon
"  for (int i=0; i<4; i++) {\n"   // edge from V[i] to  V[i+1]
"    if (V[i].y <= P.y) {\n"          // start y <= P.y
"      if (V[i+1].y > P.y) \n"      // an upward crossing
"        if (isLeft( V[i], V[i+1], P) > 0)\n"  // P left of  edge
"          ++wn;\n"            // have  a valid up intersect
"    }\n"
"    else {\n"                        // start y > P.y (no test needed)
"      if (V[i+1].y <= P.y)\n"     // a downward crossing
"        if (isLeft( V[i], V[i+1], P) < 0)\n"  // P right of  edge
"          --wn;\n"            // have  a valid down intersect
"    }\n"
"  }\n"
"  return wn;\n"
"}\n"
//===================================================================

"vec2 dist( vec2 P,  vec2 P0, vec2 P1 )\n"
// dist_Point_to_Segment(): get the distance of a point to a segment
//     Input:  a Point P and a Segment S (P0, P1) (in any dimension)
//     Return: the x,y distance from P to S
"{\n"
"  vec2 v = P1 - P0;\n"
"  vec2 w = P - P0;\n"
"  float c1 = dot(w,v);\n"
"  if ( c1 <= 0 )\n"
"    return abs(P-P0);\n"
"  float c2 = dot(v,v);\n"
"  if ( c2 <= c1 )\n"
"    return abs(P-P1);\n"
"  float b = c1 / c2;\n"
"  vec2 Pb = P0 + b * v;\n"
"  return abs(P-Pb);\n"
"}\n"

"uint pixIsInside (ivec2 Pin, uint idx){\n"
"  vec2 Quad[5];\n"
"  vec2 P;\n"
"  Quad[0] = vec2(cmd[idx].CMDXA,cmd[idx].CMDYA);\n"
"  Quad[1] = vec2(cmd[idx].CMDXB,cmd[idx].CMDYB);\n"
"  Quad[2] = vec2(cmd[idx].CMDXC,cmd[idx].CMDYC);\n"
"  Quad[3] = vec2(cmd[idx].CMDXD,cmd[idx].CMDYD);\n"
"  Quad[4] = Quad[0];\n"
"  P = vec2(Pin)/upscale;\n"

"  if (wn_PnPoly(P, Quad) != 0u) return 1u;\n"
"  else if (all(lessThanEqual(dist(P, Quad[0], Quad[1]), vec2(0.5, 0.5)))) {return 2u;}\n"
"  else if (all(lessThanEqual(dist(P, Quad[1], Quad[2]), vec2(0.5, 0.5)))) {return 3u;}\n"
"  else if (all(lessThanEqual(dist(P, Quad[2], Quad[3]), vec2(0.5, 0.5)))) {return 4u;}\n"
"  else if (all(lessThanEqual(dist(P, Quad[3], Quad[0]), vec2(0.5, 0.5)))) {return 5u;}\n"
"  else return 0u;\n"
"}\n"

"int getCmd(ivec2 P, uint id, uint start, uint end, out uint zone)\n"
"{\n"
"  for(uint i=id+start; i<id+end; i++) {\n"
"   zone = pixIsInside(P, i);\n"
"   if ( zone != 0u) {\n"
"     return int(i);\n"
"   }\n"
"  }\n"
"  return -1;\n"
"}\n"

"float cross( in vec2 a, in vec2 b ) { return a.x*b.y - a.y*b.x; }\n"


"vec2 getTexCoord(ivec2 texel, vec2 a, vec2 b, vec2 c, vec2 d) {\n"
"  vec2 p = vec2(texel)/upscale;\n"
"  vec2 e = b-a;\n"
"  vec2 f = d-a;\n"
"  vec2 h = p-a;\n"
"  vec2 g = a-b+c-d;\n"
"  if (e.x == 0.0) e.x = 1.0;\n"
"  float u = 0.0;\n"
"  float v = 0.0;\n"
"  float k1 = cross( e, f ) + cross( h, g );\n"
"  float k0 = cross( h, e );\n"
"  v = clamp(-k0/k1, 0.0, 1.0);\n"
"  u = clamp((h.x*k1+f.x*k0) / (e.x*k1-g.x*k0), 0.0, 1.0);\n"
"  return vec2( u, v );\n"
"}\n"

"vec2 getTexCoordDistorted(ivec2 texel, vec2 a, vec2 b, vec2 c, vec2 d) {\n"
//http://iquilezles.org/www/articles/ibilinear/ibilinear.htm
"  vec2 p = vec2(texel)/upscale;\n"
"  vec2 e = b-a;\n"
"  vec2 f = d-a;\n"
"  vec2 h = p-a;\n"
"  vec2 g = a-b+c-d;\n"
"  if (e.x == 0.0) e.x = 1.0;\n"
"  float u = 0.0;\n"
"  float v = 0.0;\n"
"  float k2 = cross( g, f );\n"
"  float k1 = cross( e, f ) + cross( h, g );\n"
"  float k0 = cross( h, e );\n"
"  if (abs(k2) >= 0.00001) {\n"
"    float w = k1*k1 - 4.0*k0*k2;\n"
"    if( w>=0.0 ) {\n"
"      w = sqrt( w );\n"
"      float ik2 = 0.5/k2;\n"
"      v = (-k1 - w)*ik2; \n"
"      if( v<0.0 || v>1.0 ) v = (-k1 + w)*ik2;\n"
"      u = (h.x - f.x*v)/(e.x + g.x*v);\n"
"      if( ((u<0.0) || u>(1.0)) || ((v<0.0) || v>(1.0)) ) return vec2(-1.0);\n"
"    }\n"
"  } else { \n"
"    v = clamp(-k0/k1, 0.0, 1.0);\n"
"    u = clamp((h.x*k1+f.x*k0) / (e.x*k1-g.x*k0), 0.0, 1.0);\n"
"  }\n"
"  return vec2( u, v );\n"
"}\n"

"void Vdp1ProcessSpritePixel(uint type, inout uint pixel, inout uint shadow, inout uint normalshadow, inout uint priority, inout uint colorcalc){\n"
"  shadow = 0u;\n"
"  normalshadow = 0u;\n"
"  priority = 0u;\n"
"  colorcalc = 0u;\n"
"  switch(type)\n"
"  {\n"
"    case 0x0u:\n"
"    {\n"
      // Type 0(2-bit priority, 3-bit color calculation, 11-bit color data)
"        priority = pixel >> 14;\n"
"        colorcalc = (pixel >> 11) & 0x7u;\n"
"        pixel = pixel & 0x7FFu;\n"
"        normalshadow = (pixel == 0x7FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x1u:\n"
"     {\n"
      // Type 1(3-bit priority, 2-bit color calculation, 11-bit color data)
"        priority = pixel >> 13;\n"
"        colorcalc = (pixel >> 11) & 0x3u;\n"
"        pixel &= 0x7FFu;\n"
"        normalshadow = (pixel == 0x7FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x2u:\n"
"     {\n"
        // Type 2(1-bit shadow, 1-bit priority, 3-bit color calculation, 11-bit color data)
"        shadow = pixel >> 15;\n"
"        priority = (pixel >> 14) & 0x1u;\n"
"        colorcalc = (pixel >> 11) & 0x7u;\n"
"        pixel &= 0x7FFu;\n"
"        normalshadow = (pixel == 0x7FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x3u:\n"
"     {\n"
      // Type 3(1-bit shadow, 2-bit priority, 2-bit color calculation, 11-bit color data)
"        shadow = pixel >> 15;\n"
"        priority = (pixel >> 13) & 0x3u;\n"
"        colorcalc = (pixel >> 11) & 0x3u;\n"
"        pixel &= 0x7FFu;\n"
"        normalshadow = (pixel == 0x7FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x4u:\n"
"     {\n"
      // Type 4(1-bit shadow, 2-bit priority, 3-bit color calculation, 10-bit color data)
"        shadow = pixel >> 15;\n"
"        priority = (pixel >> 13) & 0x3u;\n"
"        colorcalc = (pixel >> 10) & 0x7u;\n"
"        pixel &= 0x3FFu;\n"
"        normalshadow = (pixel == 0x3FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x5u:\n"
"     {\n"
      // Type 5(1-bit shadow, 3-bit priority, 1-bit color calculation, 11-bit color data)
"        shadow = pixel >> 15;\n"
"        priority = (pixel >> 12) & 0x7u;\n"
"        colorcalc = (pixel >> 11) & 0x1u;\n"
"        pixel &= 0x7FFu;\n"
"        normalshadow = (pixel == 0x7FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x6u:\n"
"     {\n"
      // Type 6(1-bit shadow, 3-bit priority, 2-bit color calculation, 10-bit color data)
"        shadow = pixel >> 15;\n"
"        priority = (pixel >> 12) & 0x7u;\n"
"        colorcalc = (pixel >> 10) & 0x3u;\n"
"        pixel &= 0x3FFu;\n"
"        normalshadow = (pixel == 0x3FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x7u:\n"
"     {\n"
      // Type 7(1-bit shadow, 3-bit priority, 3-bit color calculation, 9-bit color data)
"        shadow = pixel >> 15;\n"
"        priority = (pixel >> 12) & 0x7u;\n"
"        colorcalc = (pixel >> 9) & 0x7u;\n"
"        pixel &= 0x1FFu;\n"
"        normalshadow = (pixel == 0x1FEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x8u:\n"
"     {\n"
        // Type 8(1-bit priority, 7-bit color data)
"        priority = (pixel >> 7) & 0x1u;\n"
"        pixel &= 0x7Fu;\n"
"        normalshadow = (pixel == 0x7Eu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0x9u:\n"
"     {\n"
      // Type 9(1-bit priority, 1-bit color calculation, 6-bit color data)
"        priority = (pixel >> 7) & 0x1u;\n"
"        colorcalc = (pixel >> 6) & 0x1u;\n"
"        pixel &= 0x3Fu;\n"
"        normalshadow = (pixel == 0x3Eu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0xAu:\n"
"     {\n"
      // Type A(2-bit priority, 6-bit color data)
"        priority = (pixel >> 6) & 0x3u;\n"
"        pixel &= 0x3Fu;\n"
"        normalshadow = (pixel == 0x3Eu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0xBu:\n"
"     {\n"
      // Type B(2-bit color calculation, 6-bit color data)
"        colorcalc = (pixel >> 6) & 0x3u;\n"
"        pixel &= 0x3Fu;\n"
"        normalshadow = (pixel == 0x3Eu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0xCu:\n"
"     {\n"
      // Type C(1-bit special priority, 8-bit color data - bit 7 is shared)
"        priority = (pixel >> 7) & 0x1u;\n"
"        normalshadow = (pixel == 0xFEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0xDu:\n"
"     {\n"
      // Type D(1-bit special priority, 1-bit special color calculation, 8-bit color data - bits 6 and 7 are shared)
"        priority = (pixel >> 7) & 0x1u;\n"
"        colorcalc = (pixel >> 6) & 0x1u;\n"
"        normalshadow = (pixel == 0xFEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0xEu:\n"
"     {\n"
      // Type E(2-bit special priority, 8-bit color data - bits 6 and 7 are shared)
"        priority = (pixel >> 6) & 0x3u;\n"
"        normalshadow = (pixel == 0xFEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     case 0xFu:\n"
"     {\n"
      // Type F(2-bit special color calculation, 8-bit color data - bits 6 and 7 are shared)
"        colorcalc = (pixel >> 6) & 0x3u;\n"
"        normalshadow = (pixel == 0xFEu)?1u:0u;\n"
"        break;\n"
"     }\n"
"     default: break;\n"
"  }\n"
"}\n"

"uint Vdp1RamReadByte(uint addr) {\n"
"  addr &= 0x7FFFFu;\n"
"  uint read = Vdp1Ram[addr>>2];\n"
"  return (read>>(8*(addr&0x3u)))&0xFFu;\n"
"}\n"

"uint Vdp1RamReadWord(uint addr) {\n"
"  addr &= 0x7FFFFu;\n"
"  uint read = Vdp1Ram[addr>>2];\n"
"  if( (addr & 0x02u) != 0u ) { read >>= 16; } \n"
"  return (((read) >> 8 & 0xFFu) | ((read) & 0xFFu) << 8);\n"
"}\n"


"void Vdp1ReadPriority(inout cmdparameter_struct pixcmd, inout uint priority, inout uint colorcl, inout uint normal_shadow) {\n"
"  uint SPCLMD = pixcmd.SPCTL;\n"
"  uint sprite_register;\n"
"  uint reg_src = pixcmd.CMDCOLR;\n"
"  uint lutPri;\n"
"  uint not_lut = 1;\n"
  // is the sprite is RGB or LUT (in fact, LUT can use bank color, we just hope it won't...)
"  if (((SPCLMD & 0x20u)!=0x0u) && ((pixcmd.CMDCOLR & 0x8000u)!=0x0u))\n"
"  {\n"
    // RGB data, use register 0
"    priority = 0u;\n"
"    colorcl = 0u;\n"
"    return;\n"
"  }\n"
"  if (((pixcmd.CMDPMOD >> 3) & 0x7u) == 1u) {\n"
"    uint charAddr, dot, colorLut;\n"
"    priority = 0u;\n"
"    charAddr = pixcmd.CMDSRCA * 8u;\n"
//Voir ici
"    dot = Vdp1RamReadByte(charAddr);\n"
"    colorLut = pixcmd.CMDCOLR * 8u;\n"
"    lutPri = Vdp1RamReadWord((dot >> 4) * 2 + colorLut);\n"
"    if ((lutPri & 0x8000u)==0x0u) {\n"
"      not_lut = 0u;\n"
"      reg_src = lutPri;\n"
"    }\n"
"    else\n"
"      return;\n"
"  }\n"
"  {\n"
"    uint sprite_type = SPCLMD & 0xFu;\n"
"    switch (sprite_type)\n"
"    {\n"
"    case 0:\n"
"      sprite_register = (reg_src & 0xC000u) >> 14;\n"
"      priority = sprite_register;\n"
"      colorcl = (pixcmd.CMDCOLR >> 11) & 0x07u;\n"
"      normal_shadow = 0x7FEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x7FFu;\n"
"      break;\n"
"    case 1:\n"
"      sprite_register = (reg_src & 0xE000u) >> 13;\n"
"      priority = sprite_register;\n"
"      colorcl = (pixcmd.CMDCOLR >> 11) & 0x03u;\n"
"      normal_shadow = 0x7FEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x7FFu;\n"
"      break;\n"
"    case 2:\n"
"      sprite_register = (reg_src >> 14) & 0x1u;\n"
"      priority = sprite_register;\n"
"      colorcl = (pixcmd.CMDCOLR >> 11) & 0x07u;\n"
"      normal_shadow = 0x7FEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x7FFu;\n"
"      break;\n"
"    case 3:\n"
"      sprite_register = (reg_src & 0x6000u) >> 13;\n"
"      priority = sprite_register;\n"
"      colorcl = ((pixcmd.CMDCOLR >> 11) & 0x03u);\n"
"      normal_shadow = 0x7FEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x7FFu;\n"
"      break;\n"
"    case 4:\n"
"      sprite_register = (reg_src & 0x6000u) >> 13;\n"
"      priority = sprite_register;\n"
"      colorcl = (pixcmd.CMDCOLR >> 10) & 0x07u;\n"
"      normal_shadow = 0x3FEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x3FFu;\n"
"      break;\n"
"    case 5:\n"
"      sprite_register = (reg_src & 0x7000u) >> 12;\n"
"      priority = sprite_register & 0x7u;\n"
"      colorcl = (pixcmd.CMDCOLR >> 11) & 0x01u;\n"
"      normal_shadow = 0x7FEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x7FFu;\n"
"      break;\n"
"    case 6:\n"
"      sprite_register = (reg_src & 0x7000u) >> 12;\n"
"      priority = sprite_register;\n"
"      colorcl = (pixcmd.CMDCOLR >> 10) & 0x03u;\n"
"      normal_shadow = 0x3FEu;\n"
"      break;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x3FFu;\n"
"    case 7:\n"
"      sprite_register = (reg_src & 0x7000u) >> 12;\n"
"      priority = sprite_register;\n"
"      colorcl  = (pixcmd.CMDCOLR >> 9) & 0x07u;\n"
"      normal_shadow = 0x1FEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x1FFu;\n"
"      break;\n"
"    case 8:\n"
"      sprite_register = (reg_src & 0x80u) >> 7;\n"
"      priority = sprite_register;\n"
"      normal_shadow = 0x7Eu;\n"
"      colorcl = 0;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x7Fu;\n"
"      break;\n"
"    case 9:\n"
"      sprite_register = (reg_src & 0x80u) >> 7;\n"
"      priority = sprite_register;\n"
"      colorcl = ((pixcmd.CMDCOLR >> 6) & 0x01u);\n"
"      normal_shadow = 0x3Eu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x3Fu;\n"
"      break;\n"
"    case 10:\n"
"      sprite_register = (reg_src & 0xC0u) >> 6;\n"
"      priority = sprite_register;\n"
"      colorcl = 0u;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x3Fu;\n"
"      break;\n"
"    case 11:\n"
"      sprite_register = 0u;\n"
"      priority = sprite_register;\n"
"      colorcl  = (pixcmd.CMDCOLR >> 6) & 0x03u;\n"
"      normal_shadow = 0x3Eu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0x3Fu;\n"
"      break;\n"
"    case 12:\n"
"      sprite_register = (reg_src & 0x80u) >> 7;\n"
"      priority = sprite_register;\n"
"      colorcl = 0u;\n"
"      normal_shadow = 0xFEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0xFFu;\n"
"      break;\n"
"    case 13:\n"
"      sprite_register = (reg_src & 0x80u) >> 7;\n"
"      priority = sprite_register;\n"
"      colorcl = (pixcmd.CMDCOLR >> 6) & 0x01u;\n"
"      normal_shadow = 0xFEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0xFFu;\n"
"      break;\n"
"    case 14:\n"
"      sprite_register = (reg_src & 0xC0u) >> 6;\n"
"      priority = sprite_register;\n"
"      colorcl = 0u;\n"
"      normal_shadow = 0xFEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0xFFu;\n"
"      break;\n"
"    case 15:\n"
"      sprite_register = 0u;\n"
"      priority = sprite_register;\n"
"      colorcl = ((pixcmd.CMDCOLR >> 6) & 0x03u);\n"
"      normal_shadow = 0xFEu;\n"
"      if (not_lut == 1) pixcmd.CMDCOLR &= 0xFFu;\n"
"      break;\n"
"    default:\n"
"      priority = 0u;\n"
"      colorcl = 0u;\n"
"      break;\n"
"  }\n"
"}\n"
"}\n"

"vec4 VDP1COLOR(uint C, uint A, uint P, uint shadow, uint color) {\n"
"  uint col = color;\n"
"  if (C == 1u) col &= 0x17FFFu;\n"
"  else col &= 0xFFFFFFu;\n"
"  uint ret = 0x80000000u | (C << 30) | (A << 27) | (P << 24) | (shadow << 23) | col;\n"
"  return vec4(float((ret>>0)&0xFFu)/255.0,float((ret>>8)&0xFFu)/255.0,float((ret>>16)&0xFFu)/255.0,float((ret>>24)&0xFFu)/255.0);\n"
"}\n"

"uint VDP1COLOR16TO24(uint temp) {\n"
"  return (((temp & 0x1Fu) << 3) | ((temp & 0x3E0u) << 6) | ((temp & 0x7C00u) << 9)| ((temp & 0x8000u) << 1)); //Blue LSB is used for MSB bit.\n"
"}\n"

"uint VDP1MSB(uint temp) {\n"
"  return (temp & 0x7FFFu) | ((temp & 0x8000u) << 1);\n"
"}\n"

"vec4 ReadSpriteColor(cmdparameter_struct pixcmd, vec2 uv, ivec2 texel){\n"
"  vec4 color = vec4(0.0);\n"
"  uint shadow = 0;\n"
"  uint normalshadow = 0;\n"
"  uint priority = 0;\n"
"  uint colorcl = 0;\n"
"  uint endcnt = 0;\n"
"  uint normal_shadow = 0;\n"
"  uint x = uint(uv.x*pixcmd.w - 0.5);\n"
"  uint pos = (uint(pixcmd.h*uv.y - 0.5)*pixcmd.w+uint(uv.x*pixcmd.w - 0.5));\n"
"  uint charAddr = pixcmd.CMDSRCA * 8 + pos;\n"
"  uint dot;\n"
"  bool SPD = ((pixcmd.CMDPMOD & 0x40u) != 0);\n"
"  bool END = ((pixcmd.CMDPMOD & 0x80u) != 0);\n"
"  bool MSB = ((pixcmd.CMDPMOD & 0x8000u) != 0);\n"
"  uint SPCCCS = (pixcmd.SPCTL >> 12) & 0x3u;\n"
"  Vdp1ReadPriority(pixcmd, priority, colorcl, normal_shadow);\n"
"  switch ((pixcmd.CMDPMOD >> 3) & 0x7u)\n"
"  {\n"
"    case 0:\n"
"    {\n"
      // 4 bpp Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFFF0u;\n"
"      uint i;\n"
"      charAddr = pixcmd.CMDSRCA * 8 + pos/2;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"       if ((x & 0x1u) == 0u) dot = (dot>>4)&0xFu;\n"
"       else dot = (dot)&0xFu;\n"
      // Pixel 1
"      if ((dot == 0) && !SPD) color = vec4(0.0);\n"
"      else if ((dot == 0x0Fu) && !END) color = vec4(0.0);\n"
"      else if (MSB) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      }\n"
"      else if ((dot | colorBank) == normal_shadow) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      }\n"
"      else {\n"
"        uint colorindex = (dot | colorBank);\n"
"        if (((colorindex & 0x8000u)==0x8000u) && ((pixcmd.SPCTL & 0x20u)==0x20u)) {\n"
"          color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"        }else {\n"
"          color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"        }\n"
"      }\n"
"      break;\n"
"    }\n"
"    case 1:\n"
"    {\n"
      // 4 bpp LUT mode
"       uint temp;\n"
"       charAddr = pixcmd.CMDSRCA * 8 + pos/2;\n"
"       uint colorLut = pixcmd.CMDCOLR * 8;\n"
"       endcnt = 0;\n" //Ne sert pas mais potentiellement pb
"       dot = Vdp1RamReadByte(charAddr);\n"
"       if ((x & 0x1u) == 0u) dot = (dot>>4)&0xFu;\n"
"       else dot = (dot)&0xFu;\n"
"       if (!END && endcnt >= 2) {\n"
"         color = vec4(0.0);\n"
"       }\n"
"       else if ((dot == 0) && !SPD)\n"
"       {\n"
"         color = vec4(0.0);\n"
"       }\n"
"       else if ((dot == 0x0Fu) && !END)\n" // 6. Commandtable end code
"       {\n"
"         color = vec4(0.0);\n"
"         endcnt++;\n"
"       }\n"
"       else {\n"
"         temp = Vdp1RamReadWord((dot * 2 + colorLut));\n"
"         if ((temp & 0x8000u)==0x8000u) {\n"
"           if (MSB) {\n"
"             color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"           } else {\n"
"             color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(temp));\n"
"           }\n"
"         } else if (temp != 0x0000u) {\n"
"           Vdp1ProcessSpritePixel(pixcmd.SPCTL & 0xFu, temp, shadow, normalshadow, priority, colorcl);\n"
"           if (shadow != 0) {\n"
"             color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"           }\n"
"           else {\n"
"             if (normalshadow != 0) {\n"
"               color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"             }\n"
"             else {\n"
"               uint colorindex = temp;\n"
"               if (((colorindex & 0x8000u)==0x8000u) && ((pixcmd.SPCTL & 0x20u)==0x20u)) {\n"
"                 color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"               }\n"
"               else {\n"
"                 color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"               }\n"
"             }\n"
"           }\n"
"         } else {\n"
"           color = VDP1COLOR(1, colorcl, priority, 0, 0);\n"
"         }\n"
"       }\n"
"       break;\n"
"    }\n"
"    case 2:\n"
"    {\n"
      // 8 bpp(64 color) Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFFC0u;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"      if ((dot == 0) && !SPD) color = vec4(0.0);\n"
"      else if ((dot == 0xFFu) && !END) color = vec4(0.0);\n"
"      else if (MSB) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      }\n"
"      else if (((dot & 0x3Fu) | colorBank) == normal_shadow) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      }\n"
"      else {\n"
"        uint colorindex = ((dot & 0x3Fu) | colorBank);\n"
"        if (((colorindex & 0x8000u)==0x8000u) && ((pixcmd.SPCTL & 0x20u)==0x20u)) {\n"
"          color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"        }\n"
"        else {\n"
"          color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"        }\n"
"      }\n"
"      break;\n"
"    }\n"
"    case 3:\n"
"    {\n"
      // 8 bpp(128 color) Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFF80u;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"      if ((dot == 0) && !SPD) color = vec4(0.0);\n"
"      else if ((dot == 0xFFu) && !END) color = vec4(0.0);\n"
"      else if (MSB) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      }\n"
"      else if (((dot & 0x7Fu) | colorBank) == normal_shadow) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      }\n"
"      else {\n"
"        uint colorindex = ((dot & 0x7Fu) | colorBank);\n"
"        if (((colorindex & 0x8000u)==0x8000u) && ((pixcmd.SPCTL & 0x20u)==0x20u)) {\n"
"          color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"        }\n"
"        else {\n"
"          color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"        }\n"
"      }\n"
"      break;\n"
"    }\n"
"    case 4:\n"
"    {\n"
      // 8 bpp(256 color) Bank mode
"      uint colorBank = pixcmd.CMDCOLR & 0xFF00u;\n"
"      dot = Vdp1RamReadByte(charAddr);\n"
"      if ((dot == 0) && !SPD) {\n"
"        color = vec4(0.0);\n"
"      } else if ((dot == 0xFFu) && !END) {\n"
"        color = vec4(0.0);\n"
"      } else if (MSB) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      } else if ((dot | colorBank) == normal_shadow) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      } else {\n"
"        uint colorindex = (dot | colorBank);\n"
"        if (((colorindex & 0x8000u)==0x8000u) && ((pixcmd.SPCTL & 0x20u)==0x20u)) {\n"
"          color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"        } else {\n"
"          color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"        }\n"
"      }\n"
"      break;\n"
"    }\n"
"    case 5:\n"
"    {\n"
      // 16 bpp Bank mode
"      uint temp;\n"
"      charAddr += pos;\n"
"      temp = Vdp1RamReadWord(charAddr);\n"
"      if (((temp & 0x8000u)==0x0u) && !SPD) {\n"
"        color = vec4(0.0);\n"
"      }\n"
"      else if ((temp == 0x7FFFu) && !END) {\n"
"        color = vec4(0.0);\n"
"      }\n"
"      else if (MSB || (normal_shadow!=0 && temp == normal_shadow) ) {\n"
"        color = VDP1COLOR(1, 0, 0, 1, 0);\n"
"      }\n"
"      else {\n"
"        if (((temp & 0x8000u)==0x8000u) && ((pixcmd.SPCTL & 0x20u)==0x20u)) {\n"
"          color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(temp));\n"
"        }\n"
"        else {\n"
"          Vdp1ProcessSpritePixel(pixcmd.SPCTL & 0xFu, temp, shadow, normalshadow, priority, colorcl);\n"
"          color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(temp) );\n"
"        }\n"
"      }\n"
"      break;\n"
"    }\n"
"    default:\n"
"      break;\n"
"  }\n"
"  return color;\n"
"}\n"

"vec4 ReadPolygonColor(cmdparameter_struct pixcmd){\n"
"  vec4 color = vec4(0.0);\n"
"  uint shadow = 0u;\n"
"  uint normalshadow = 0u;\n"
"  uint normal_shadow = 0u;\n"
"  uint priority = 0u;\n"
"  uint colorcl = 0u;\n"
"  bool END = ((pixcmd.CMDPMOD & 0x80u) != 0u);    // end-code disable(ECD) hard/vdp1/hon/p06_34.htm\n"
"  bool MSB = ((pixcmd.CMDPMOD & 0x8000u) != 0u);\n"
"  uint SPCCCS = (pixcmd.SPCTL >> 12) & 0x3u;\n"
"  if ((pixcmd.CMDCOLR & 0x8000u)!=0u && // Sprite Window Color\n"
"   (pixcmd.SPCTL & 0x10u)!=0u && // Sprite Window is enabled\n"
"   (pixcmd.CMDPMOD & 4u)==0u &&\n"
"   (((pixcmd.CMDPMOD >> 3) & 0x7u) < 5u)  && //Is palette\n"
"   ((pixcmd.SPCTL & 0xFu)>=2u && (pixcmd.SPCTL & 0xFu) < 8u)) // inside sprite type\n"
"  {\n"
"    return vec4(0.0);\n"
"  }\n"
"  Vdp1ReadPriority(pixcmd, priority, colorcl, normal_shadow);\n"
"  switch ((pixcmd.CMDPMOD >> 3) & 0x7u)\n"
"  {\n"
"  case 0:\n"
"  {\n"
"    // 4 bpp Bank mode\n"
"    uint colorBank = pixcmd.CMDCOLR;\n"
"    if (MSB || colorBank == normal_shadow) {\n"
"      color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"    } else {\n"
"      uint colorindex = (colorBank);\n"
"      if ((colorindex & 0x8000u)==0x8000u) {\n"
"        color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"      } else {\n"
"        color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"      }\n"
"    }\n"
"    break;\n"
"  }\n"
"  case 1:\n"
"  {\n"
  // 4 bpp LUT mode
"    uint temp;\n"
"    uint colorLut = pixcmd.CMDCOLR * 8;\n"

"    if (pixcmd.CMDCOLR == 0u) return vec4(0.0);\n"

  // RBG and pallet mode
"    if ( (pixcmd.CMDCOLR & 0x8000u)!=0u && (pixcmd.SPCTL & 0x20u)!=0u) {\n"
"      color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(pixcmd.CMDCOLR));\n"
"      return color;\n"
"    }\n"
"    temp = Vdp1RamReadWord(colorLut);\n"
"    if ((temp & 0x8000u)!=0u) {\n"
"      if (MSB) color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      else color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(temp));\n"
"    }\n"
"    else if (temp != 0x0u) {\n"
"      Vdp1ProcessSpritePixel(pixcmd.SPCTL & 0xFu, temp, shadow, normalshadow, priority, colorcl);\n"
"      uint colorBank = temp;\n"
"      if (MSB || (shadow==1u)) {\n"
"        color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"      }\n"
"      else {\n"
"        uint colorindex = (colorBank);\n"
"        if ((colorindex & 0x8000u)!=0u) {\n"
"          color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"        }\n"
"        else {\n"
"          color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"        }\n"
"      }\n"
"    }\n"
"    else {\n"
"      color = VDP1COLOR(1, colorcl, priority, 0, 0);\n"
"    }\n"
"    break;\n"
"  }\n"
"  case 2: {\n"
     // 8 bpp(64 color) Bank mode
"    uint colorBank = pixcmd.CMDCOLR & 0xFFC0u;\n"
"    if ( MSB || colorBank == normal_shadow) {\n"
"      color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"    } else {\n"
"      uint colorindex = colorBank;\n"
"      if ((colorindex & 0x8000u)!=0u) {\n"
"        color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"      }\n"
"      else {\n"
"        color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"      }\n"
"    }\n"
"    break;\n"
"  }\n"
"  case 3: {\n"
  // 8 bpp(128 color) Bank mode
"    uint colorBank = pixcmd.CMDCOLR & 0xFF80u;\n"
"    if (MSB || colorBank == normal_shadow) {\n"
"      color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"    } else {\n"
"      uint colorindex = (colorBank);\n"
"      if ((colorindex & 0x8000u)!=0u) {\n"
"        color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"      }\n"
"      else {\n"
"        color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"      }\n"
"    }\n"
"    break;\n"
"  }\n"
"  case 4: {\n"
  // 8 bpp(256 color) Bank mode
"    uint colorBank = pixcmd.CMDCOLR;\n"
"    if ( MSB || colorBank == normal_shadow) {\n"
"      color = VDP1COLOR(1, 0, priority, 1, 0);\n"
"    }\n"
"    else {\n"
"      uint colorindex = (colorBank);\n"
"      if ((colorindex & 0x8000u) != 0u) {\n"
"        color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(colorindex));\n"
"      }\n"
"      else {\n"
"        color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(colorindex));\n"
"      }\n"
"    }\n"
"    break;\n"
"  }\n"
"  case 5:\n"
"  case 6:\n"
"  {\n"
"    // 16 bpp Bank mode\n"
"    uint dot = pixcmd.CMDCOLR;\n"
"    if (dot == 0x0000u) {\n"
"      color = VDP1COLOR(1, 0, 0, 0, 0);\n"
"    }\n"
"    else if ((dot == 0x7FFFu) && !END) {\n"
"      color = VDP1COLOR(1, 0, 0, 0, 0);\n"
"    }\n"
"    else if (MSB || dot == normal_shadow) {\n"
"      color = VDP1COLOR(1, 0, 0, 1, 0);\n"
"    }\n"
"    else {\n"
"      if ((dot & 0x8000u)!=0u) {\n"
"        color = VDP1COLOR(0, colorcl, 0, 0, VDP1COLOR16TO24(dot));\n"
"      }\n"
"      else {\n"
"        color = VDP1COLOR(1, colorcl, priority, 0, VDP1MSB(dot));\n"
"      }\n"
"    }\n"
"  }\n"
"  break;\n"
"  default:\n"
"    color = vec4(0.0);\n"
"    break;\n"
"  }\n"
"  return color;\n"
"}\n"

"void main()\n"
"{\n"
"  vec4 lastColor = vec4(0.0);\n"
"  cmdparameter_struct pixcmd;\n"
"  uint discarded = 0;\n"
"  vec2 texcoord = vec2(0);\n"
"  vec4 finalColor = vec4(0.0);\n"
"  ivec2 size = imageSize(outSurface);\n"
"  ivec2 texel = ivec2(gl_GlobalInvocationID.xy);\n"
"  if (texel.x >= size.x || texel.y >= size.y ) return;\n"
"  ivec2 index = ivec2((texel.x*"Stringify(NB_COARSE_RAST_X)")/size.x, (texel.y*"Stringify(NB_COARSE_RAST_Y)")/size.y);\n"
"  uint lindex = index.y*"Stringify(NB_COARSE_RAST_X)"+ index.x;\n"
"  uint cmdIndex = lindex * 2000u;\n"

"  if (nbCmd[lindex] == 0u) return;\n"
"  uint idCmd = 0;\n"
"  uint zone = 0;\n"
"  int cmdindex = 0;\n"
"  while ((cmdindex != -1) && (lastColor == vec4(0.0)) && (idCmd<nbCmd[lindex]) ) {\n"
"    cmdindex = getCmd(texel, cmdIndex, idCmd, nbCmd[lindex], zone);\n"
"    idCmd = cmdindex + 1 - cmdIndex;\n"
"    if (cmdindex == -1) continue;\n"
"    pixcmd = cmd[cmdindex];\n"
"    if (pixcmd.type == "Stringify(POLYGON)") {\n"
"      texcoord = getTexCoordDistorted(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      if ((texcoord.x == -1.0) && (texcoord.y == -1.0))lastColor = vec4(0.0);\n"
"      else {\n"
"        if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"        if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
"        lastColor = ReadPolygonColor(pixcmd);\n"
"      }\n"
"    } else if (pixcmd.type == "Stringify(DISTORTED)") {\n"
"      texcoord = getTexCoordDistorted(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      if ((texcoord.x == -1.0) && (texcoord.y == -1.0))lastColor = vec4(0.0);\n"
"      else {\n"
"        if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"        if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
"        lastColor = ReadSpriteColor(pixcmd, texcoord, texel);\n"
"      }\n"
"    } else if (pixcmd.type == "Stringify(NORMAL)") {\n"
"      texcoord = getTexCoord(texel, vec2(pixcmd.P[0],pixcmd.P[1])/2.0, vec2(pixcmd.P[2],pixcmd.P[3])/2.0, vec2(pixcmd.P[4],pixcmd.P[5])/2.0, vec2(pixcmd.P[6],pixcmd.P[7])/2.0);\n"
"      if ((pixcmd.flip & 0x1u) == 0x1u) texcoord.x = 1.0 - texcoord.x;\n" //invert horizontally
"      if ((pixcmd.flip & 0x2u) == 0x2u) texcoord.y = 1.0 - texcoord.y;\n" //invert vertically
"      lastColor = ReadSpriteColor(pixcmd, texcoord, texel);\n"
"    }\n"
"    if ((pixcmd.CMDPMOD & 0x100u)==0x100u){\n"//IS_MESH
     //Implement Mesh shader (Duplicate for improve)
     //Normal mesh for the moment
"      if( (texel.y & 0x01) == 0 ){ \n"
"        if( (texel.x & 0x01) == 0 ){ \n"
"          lastColor = vec4(0.0);\n"
"        }\n"
"      }else{ \n"
"        if( (texel.x & 0x01) == 1 ){ \n"
"          lastColor = vec4(0.0);\n"
"        } \n"
"      } \n"
"    } else if ((pixcmd.CMDPMOD & 0x8000u)!=0x00u){\n"//IS_MSB
//Implement PG_VDP1_MSB
"    } else if ((pixcmd.CMDPMOD & 0x03u)==0x00u){\n" //REPLACE
//Implement PG_VDP1_GOURAUDSHADING
"    } else if ((pixcmd.CMDPMOD & 0x03u)==0x01u){\n"//IS_DONOT_DRAW_OR_SHADOW
//Implement PG_VDP1_SHADOW
"    } else if ((pixcmd.CMDPMOD & 0x03u)==0x02u){\n"//IS_HALF_LUMINANCE
//Implement PG_VDP1_HALF_LUMINANCE
"    } else if ((pixcmd.CMDPMOD & 0x03u)==0x03u){\n"//IS_REPLACE_OR_HALF_TRANSPARENT
//Implement PG_VDP1_GOURAUDSHADING_HALFTRANS
"    }\n"
"  }\n"
#ifdef SHOW_QUAD
"  if (zone > 1u) lastColor = VDP1COLOR(0, 0, 0, 0, 0xFF);\n"
#endif
"  if (lastColor == vec4(0.0)) return;\n"
"  finalColor = lastColor;\n";
static const char vdp1_end_f[] =
"  if ((pixcmd.CMDPMOD & 0x4u) == 0x4u) {\n"
"    finalColor.r = clamp(finalColor.r + mix(mix(pixcmd.G[0],pixcmd.G[4],texcoord.x), mix(pixcmd.G[12],pixcmd.G[8],texcoord.x), texcoord.y), 0.0, 1.0);\n"
"    finalColor.g = clamp(finalColor.g + mix(mix(pixcmd.G[1],pixcmd.G[5],texcoord.x), mix(pixcmd.G[13],pixcmd.G[9],texcoord.x), texcoord.y), 0.0, 1.0);\n"
"    finalColor.b = clamp(finalColor.b + mix(mix(pixcmd.G[2],pixcmd.G[6],texcoord.x), mix(pixcmd.G[14],pixcmd.G[10],texcoord.x), texcoord.y), 0.0, 1.0);\n"
"  }\n"
"  imageStore(outSurface,ivec2(texel.x,size.y-texel.y),finalColor);\n"
"}\n";

static const char vdp1_test_f[] = "";
// "  uint col = color[uint(texcoord.y * colHeight)*  colWidth + uint(texcoord.x * colWidth)];\n"
// "  a = (0x80 | 0x0)/255.0f;\n"//float((col>>24u)&0xFFu)/255.0f;\n"
// "  b = float((col>>16u)&0xFFu)/255.0f;\n"
// "  g = float((col>>8u)&0xFFu)/255.0f;\n"
// "  r = float((col>>0u)&0xFFu)/255.0f;\n";

static const char vdp1_0_Pal_f[] = "";
static const char vdp1_0_Mix_f[] =
"\n";

#endif //VDP1_PROG_COMPUTE_H
