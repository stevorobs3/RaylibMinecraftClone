#include "texture_atlas.h"
#include <algorithm>
#include <string>
#include <external/glad.h>

std::array<Vector2,4> TextureAtlas::lookup_texture_uv(TextureId id) const noexcept {
  const int index=static_cast<int>(id);
  const float u=static_cast<float>(index%grid_size)/grid_size;
  const float v=static_cast<float>(index/grid_size)/grid_size;
  const float step=1.0f/grid_size;
  return {{{u,v},{u+step,v},{u,v+step},{u+step,v+step}}};
}

ImageMips TextureAtlas::load_image_with_mips(const fs::path& path) {
  ImageMips mips{};
  const std::string file=path.string();
  mips[0].reset(LoadImage(file.c_str()));
  if(!mips[0].valid()) {
    TraceLog(LOG_ERROR,"Failed to load image: %s",file.c_str());
    return {};
  }
  ImageFormat(&mips[0].image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
  if(!mips[0].valid()) return {};
  for(int i=1;i<MIP_LEVELS;++i) {
    const Image& prev=mips[i-1].image;
    mips[i].reset(ImageCopy(prev));
    if(!mips[i].valid()) return {};
    ImageResize(&mips[i].image,std::max(1,prev.width/2),std::max(1,prev.height/2));
    if(!mips[i].valid()) return {};
  }
  return mips;
}

bool TextureAtlas::same_size(const AllImageMips& all,int level) noexcept {
  const Image& first=all[0][level].image;
  if(!IsImageValid(first)) return false;
  for(const auto& mips:all) {
    const Image& image=mips[level].image;
    if(!IsImageValid(image) || image.width!=first.width ||
       image.height!=first.height || image.format!=first.format) return false;
  }
  return true;
}
std::optional<ImageMips> TextureAtlas::stitch_images(const AllImageMips& all) {
  ImageMips output{};
  for(int level=0;level<MIP_LEVELS;++level) {
    if(!same_size(all,level)) return std::nullopt;
    const Image& first=all[0][level].image;
    output[level].reset(GenImageColor(first.width*grid_size,first.height*grid_size,BLANK));
    if(!output[level].valid()) return std::nullopt;
    for(int i=0;i<static_cast<int>(file_names.size());++i) {
      const Image& source=all[i][level].image;
      ImageDraw(&output[level].image,source,
        Rectangle{0,0,static_cast<float>(source.width),static_cast<float>(source.height)},
        Rectangle{static_cast<float>(i%grid_size*source.width),
                  static_cast<float>(i/grid_size*source.height),
                  static_cast<float>(source.width),static_cast<float>(source.height)},
        WHITE);
    }
  }
  return output;
}
Texture2D TextureAtlas::to_texture(const ImageMips& mips) noexcept {
  GLuint id=0;
  glGenTextures(1,&id);
  if(id==0) return {};
  glBindTexture(GL_TEXTURE_2D,id);
  for(int level=0;level<MIP_LEVELS;++level) {
    const Image& image=mips[level].image;
    glTexImage2D(GL_TEXTURE_2D,level,GL_RGBA8,image.width,image.height,0,
                 GL_RGBA,GL_UNSIGNED_BYTE,image.data);
  }
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,MIP_LEVELS-1);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D,0);
  return Texture2D{id,mips[0].image.width,mips[0].image.height,
                   MIP_LEVELS,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
}
bool TextureAtlas::load() {
  AllImageMips all{};
  const auto directory=fs::path(GetApplicationDirectory())/"assets";
  for(int i=0;i<static_cast<int>(file_names.size());++i) {
    all[i]=load_image_with_mips(directory/file_names[i]);
    if(!all[i][0].valid()) return false;
  }
  auto stitched=stitch_images(all);
  if(!stitched) return false;
  const Texture2D replacement=to_texture(*stitched);
  if(replacement.id==0) return false;
  unload();
  texture_=replacement;
  return true;
}
void TextureAtlas::unload() noexcept {
  if(texture_.id!=0) UnloadTexture(texture_);
  texture_={};
}
