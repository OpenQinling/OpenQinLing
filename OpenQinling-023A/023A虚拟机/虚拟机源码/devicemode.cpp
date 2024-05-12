#include "devicemode.h"

DeviceMode::DeviceMode()
{

}


//生成iconMap
void DeviceMode::crateIconMap(){
    iconMap = ToolFunction::crateIconMap(devType,devName,beginAddress,useMemLength,useInterNum);
}
