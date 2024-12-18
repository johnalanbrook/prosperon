# Requires shadercross CLI installed from SDL_shadercross
for filename in *.vert.hlsl; do
    if [ -f "$filename" ]; then
        echo "compiling ${filename}"
        shadercross "$filename" -o "spirv/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "msl/${filename/.hlsl/.msl}"
        shadercross "$filename" -o "dxil/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.frag.hlsl; do
    if [ -f "$filename" ]; then
        echo "compiling ${filename}"
        shadercross "$filename" -o "spirv/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "msl/${filename/.hlsl/.msl}"
        shadercross "$filename" -o "dxil/${filename/.hlsl/.dxil}"
    fi
done

for filename in *.comp.hlsl; do
    if [ -f "$filename" ]; then
        echo "compiling ${filename}"
        shadercross "$filename" -o "spirv/${filename/.hlsl/.spv}"
        shadercross "$filename" -o "msl/${filename/.hlsl/.msl}"
        shadercross "$filename" -o "dxil/${filename/.hlsl/.dxil}"
    fi
done
