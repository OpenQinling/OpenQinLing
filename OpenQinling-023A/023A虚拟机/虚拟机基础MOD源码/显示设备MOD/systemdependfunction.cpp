#include "systemdependfunction.h"
#include <windows.h>

void SystemDependFunction::setMousePos(int x,int y){
    SetCursorPos(x,y);
}

void SystemDependFunction::getMousePos(int&x,int&y){
    POINT point;
    GetCursorPos(&point);
    x = point.x;
    y = point.y;
    ShowCursor(false);
}
