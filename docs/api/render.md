# render
Draw shapes in screen space.
#### flushtext()



#### camera_screen2world()



#### viewport()



#### end_pass()



#### commit()



#### glue_pass()



#### text_size()



#### text_ssbo()



#### set_camera()



#### pipeline()



#### setuniv3()



#### setuniv()



#### spdraw()



#### setuniproj()



#### setuniview()



#### setunivp()



#### setunim4()



#### setuniv2()



#### setuniv4()



#### setpipeline()



#### screencolor()



#### imgui_new()



#### gfx_gui()



#### imgui_end()



#### imgui_init()



#### poly_prim(verts)



#### make_shader(shader)



#### shader_apply_material(shader, material = {})



#### sg_bind(shader, mesh = {}, material = {}, ssbo)



#### device
**object**

Device resolutions given as [x,y,inches diagonal].

#### init()



#### circle(pos, radius, color)



#### poly(points, color, transform)



#### line(points, color = Color.white, thickness = 1, transform)



#### point(pos,size,color = Color.blue)



#### cross(pos, size, color = Color.red)

Draw a cross centered at pos, with arm length size.

#### arrow(start, end, color = Color.red, wingspan = 4, wingangle = 10)

Draw an arrow from start to end, with wings of length wingspan at angle wingangle.

#### coordinate(pos, size, color)



#### boundingbox(bb, color = Color.white)



#### rectangle(lowerleft, upperright, color)

Draw a rectangle, with its corners at lowerleft and upperright.

#### box(pos, wh, color = Color.white)



#### window(pos, wh, color)



#### text(str, pos, size = 1, color = Color.white, wrap = -1, anchor = [0,1], cursor = -1)



#### image(tex, pos, scale = 1, rotation = 0, color = Color.white, dimensions = [tex.width, tex.height])



#### fontcache
**object**



#### set_font(path, size)




