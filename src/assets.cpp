// Menu source watermark: @use_sberbank
#include "assets.h"
#include "font_data.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
std::vector<unsigned char> load(const char* name){
 std::ifstream file(std::string(MENU_ASSET_DIR)+"/"+name,std::ios::binary);
 if(!file)throw std::runtime_error(std::string("Missing UI asset: ")+name);
 return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
const auto avatar=load("avatar.rgba"),inventory=load("inventory.pak");
}
namespace nl::assets {
const Blob noto_sans_medium{font_data::medium,font_data::mediumSize};
const Blob noto_sans_semibold{font_data::semibold,font_data::semiboldSize};
const Blob noto_sans_black{font_data::black,font_data::blackSize};
const Blob fa_solid{font_data::icons,font_data::iconsSize};
const Blob avatar_rgba{avatar.data(),avatar.size()};
const Blob inventory_pak{inventory.data(),inventory.size()};
}
