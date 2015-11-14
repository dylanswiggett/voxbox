#version 430 core

layout(location = 0) in vec3 vertexPos;

void main() {
  gl_Position = vec4(vertexPos.xy, 0, 0);
}
