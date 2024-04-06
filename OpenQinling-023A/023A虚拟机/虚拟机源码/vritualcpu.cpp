#include "vritualcpu.h"

VritualCPU::VritualCPU(function<void(void)>StartDevice,
           function<void(void)>StopDevice,
           function<void(void)>BlackoutDevice,
           function<void(void)>EnergizeDevice,
           function<void(uint8_t data,uint32_t add)>WriteDevice,
           function<uint8_t(uint32_t add)>ReadDevice,
           function<uint8_t(uint8_t &num)>GetIsHaveInterruptAsk,
           function<void(void)>cpuStopCallBack){
    runStep = -2;
    for(int i = 0;i<15;i++){
        regGroup[i] = 0;
    }
    regGroup[pc] = 1024;
    if(StartDevice!=0){
        this->StartDevice = StartDevice;
    }
    if(StopDevice!=0){
        this->StopDevice = StopDevice;
    }
    if(BlackoutDevice!=0){
        this->BlackoutDevice = BlackoutDevice;
    }
    if(EnergizeDevice!=0){
        this->EnergizeDevice = EnergizeDevice;
    }
    if(WriteDevice!=0){
        this->WriteDevice = WriteDevice;
    }
    if(ReadDevice!=0){
        this->ReadDevice = ReadDevice;
    }
    if(GetIsHaveInterruptAsk!=0){
        this->GetIsHaveInterruptAsk = GetIsHaveInterruptAsk;
    }
    if(cpuStopCallBack!=0){
        this->cpuStopCallBack = cpuStopCallBack;
    }

    cpuRunThread = QThread::create([=](){
        QElapsedTimer t;
        while (!isDeleteCpuObj) {
            int tmp = runStep;
            if(tmp>0 || tmp==-1){
                runOwnStep();
                if(tmp!=-1)runStep--;
            }if(tmp==0){
                this->cpuStopCallBack();
                runStep = -2;
            }

        }
    });
    cpuRunThread->start();
}


VritualCPU::~VritualCPU(){
    isDeleteCpuObj = 1;
    stop();
    while(!cpuRunThread->isFinished());
    delete cpuRunThread;
}

void VritualCPU::run(int step){
    if(runStep==-2){
        StartDevice();
        runStep = step;
    }
}

void VritualCPU::restart(){
    stop();

    for(int i = 0;i<15;i++){
        regGroup[i] = 0;
    }
    regGroup[pc] = 1024;

    BlackoutDevice();
    EnergizeDevice();
}

void VritualCPU::stop(){
    if(runStep!=-2 && runStep!=0){
        runStep = 0;
        while(runStep!=-2){
            QThread::msleep(5);
        }
        StopDevice();
    }
}

uint32_t VritualCPU::ReadDeviceFun(uint32_t add,RwDatas size){
    if(size==RW_BYTE){
        return ReadDevice(add);
    }else if(size==RW_WORD){
        WORD d(ReadDevice(add),ReadDevice(add+1));
        return d.toUShort();
    }else if(size==RW_DWORD){
        DWORD d(ReadDevice(add),ReadDevice(add+1),ReadDevice(add+2),ReadDevice(add+3));
        return d.toUInt();
    }
}

void VritualCPU::WriteDeviceFun(uint32_t data,uint32_t add,RwDatas size){
    if(size==RW_BYTE){
        WriteDevice(data,add);
    }else if(size==RW_WORD){
        WORD d = (uint16_t)data;
        QByteArray tmp = d.toByteArray();
        WriteDevice(tmp[0],add);
        WriteDevice(tmp[1],add+1);
    }else if(size==RW_DWORD){
        DWORD d = data;
        QByteArray tmp = d.toByteArray();
        WriteDevice(tmp[0],add);
        WriteDevice(tmp[1],add+1);
        WriteDevice(tmp[2],add+2);
        WriteDevice(tmp[3],add+3);
    }
}

uint32_t VritualCPU::getVirtualPageInfo(uint32_t virAdd,bool &isHavePage,bool &pageIsInStorge,bool &isReadOnlyPage){
    uint32_t tlbAdd = regGroup[tlb];
    tlbAdd += (virAdd>>4)*4;//获取虚拟内存页对应物理内存页的地址
    uint32_t pageInfo = ReadDeviceFun(tlbAdd,RW_DWORD);//读取内存页映射信息
    isHavePage = ToolFunction::getBitsData(pageInfo,31,31);
    isReadOnlyPage = ToolFunction::getBitsData(pageInfo,30,30);
    pageIsInStorge = ToolFunction::getBitsData(pageInfo,29,29);
    return ToolFunction::getBitsData(pageInfo,27,0)<<4 | ToolFunction::getBitsData(virAdd,3,0);
}

void VritualCPU::runOwnStep(){
    //当前的cpu状态信息
    uint32_t isInter = ToolFunction::getBitsData(regGroup[sys],0,0);//是否开启了中断
    uint32_t isProtect = ToolFunction::getBitsData(regGroup[sys],1,1);//是否开启了保护模式
    uint32_t isVirtualMem = ToolFunction::getBitsData(regGroup[sys],2,2);//是否开启了虚拟内存

    if(isInter){
        uint8_t num;
        if(GetIsHaveInterruptAsk(num)){
            regGroup[ipc] = regGroup[pc];
            regGroup[sys] = 0;
            regGroup[pc] = ReadDeviceFun(num*4,RW_DWORD);
            return;
        }
    }


    //MMU占用的中断号: 15=缺页 14=访问非法地址
#define ReadVirtualMem(data,virAdd,Bytes) do{    \
    bool isHavePage,isReadOnlyPage,pageIsInStorge;\
    uint32_t add = getVirtualPageInfo(virAdd,isHavePage,isReadOnlyPage,pageIsInStorge);\
    if(!isHavePage || pageIsInStorge){\
        WriteDeviceFun(virAdd>>4,65532,RW_DWORD);\
        regGroup[ipc] = regGroup[pc];\
        int num = !isHavePage ? 15 : 14;\
        regGroup[pc] = ReadDeviceFun(num*4,RW_DWORD);\
        return;\
    }\
    data =  ReadDeviceFun(add,Bytes);\
}while(0)

#define    WriteVirtualMem(data,virAdd,Bytes) do{\
    bool isHavePage,isReadOnlyPage,pageIsInStorge;\
    uint32_t add = getVirtualPageInfo(virAdd,isHavePage,isReadOnlyPage,pageIsInStorge);\
    if(!isHavePage || isReadOnlyPage || pageIsInStorge){\
        WriteDeviceFun(virAdd>>4,65532,RW_DWORD);\
        regGroup[ipc] = regGroup[pc];\
        int num = (isReadOnlyPage||!isHavePage) ? 14 : 15;\
        regGroup[pc] = ReadDeviceFun(num*4,RW_DWORD);\
        return;\
    }\
    WriteDeviceFun(data,add,Bytes);\
}while(0);

    uint32_t order;
    if(isVirtualMem){//读虚拟内存
        ReadVirtualMem(order,regGroup[pc],RW_DWORD);
    }else{//读物理内存
        order =  ReadDeviceFun(regGroup[pc],RW_DWORD);
    }

    uint32_t orderType = ToolFunction::getBitsData(order,31,27);

    if(orderType==10){//条件跳转JNE
        if(regGroup[flag]==0){
            regGroup[pc] = ToolFunction::getBitsData(regGroup[pc],31,27)<<27 | ToolFunction::getBitsData(order,26,0);
        }else{
            regGroup[pc] = regGroup[pc] + 4;
        }
    }else if(orderType==11){//无条件短距跳转JMP
        regGroup[pc] = ToolFunction::getBitsData(regGroup[pc],31,27)<<27 | ToolFunction::getBitsData(order,26,0);
    }else if(orderType==12){//无条件长距跳转JT
        regGroup[pc] = ToolFunction::getBitsData(regGroup[r7],4,0)<<27 | ToolFunction::getBitsData(order,26,0);
    }else if(orderType==13){//请求软中断
        if(isInter){//当前关闭了中断响应,则不处理软中断
             regGroup[pc] = regGroup[pc]+4;
        }else{
            uint32_t num = ToolFunction::getBitsData(order,7,0);
            if(num<=15){
                num = 1;
            }
            regGroup[ipc] = regGroup[pc]+4;

            regGroup[pc] = ReadDeviceFun(num*4,RW_DWORD);
        }
    }else if(orderType==14){//链接跳转
        uint32_t subType = ToolFunction::getBitsData(order,26,0);
        if(subType==0){
            //函数链接跳转
            uint32_t tmp = regGroup[pc]+4;
            regGroup[pc] = regGroup[tpc];
            regGroup[tpc] = tmp;
        }else{
            //中断链接跳转
            uint32_t tmp = regGroup[pc]+4;
            regGroup[pc] = regGroup[ipc];
            regGroup[ipc] = tmp;
            if(subType==2){//开启中断
                regGroup[sys] = isVirtualMem<<2 | isProtect<<1 | 1;
            }else if(subType==3){//开启中断和虚拟内存
                regGroup[sys] = isVirtualMem<<2 | 3;
            }else if(subType==4){//开启中断/虚拟内存/保护模式
                regGroup[sys] = 7;
            }
        }
    }else if(orderType==15){//cpu权限设置
        uint32_t mode = ToolFunction::getBitsData(order,26,0);
        if(isProtect && (mode==1 || mode==4)){
            //保护模式下不允许关闭虚拟内存或者关闭中断响应,否则触发软中断8
            if(isInter){
                regGroup[ipc] = regGroup[pc]+4;
                int num = 8;
                regGroup[pc] = ReadDeviceFun(num*4,RW_DWORD);
            }else{
                regGroup[pc] = regGroup[pc]+4;
            }
            return;
        }
        if(mode==0){
            //开启中断响应
            regGroup[sys] = isVirtualMem<<2 | isProtect<<1 | 1;
            regGroup[pc] = regGroup[pc]+4;
        }else if(mode==1){
            //关闭中断响应
            regGroup[sys] = isVirtualMem<<2 | isProtect<<1;
            regGroup[pc] = regGroup[pc]+4;
        }else if(mode==2){
            //开启保护模式
            regGroup[sys] = isVirtualMem<<2 | 1 | isInter;
            regGroup[pc] = regGroup[pc]+4;
        }else if(mode==3){
            //开启虚拟内存
            regGroup[sys] = 1 | isProtect<<1 | isInter;
            regGroup[pc] = regGroup[pc]+4;
        }else if(mode==4){
            //关闭虚拟内存
            regGroup[sys] = isProtect<<1 | isInter;
            regGroup[pc] = regGroup[pc]+4;
        }else{
            //出错
            if(isInter){
                regGroup[ipc] = regGroup[pc]+4;
                int num = 3;
                regGroup[pc] = ReadDeviceFun(num*4,RW_DWORD);
            }else{
                regGroup[pc] = regGroup[pc]+4;
            }
        }
    }else if(orderType==31){//重启cpu
        for(int i = 0;i<15;i++){
            regGroup[i] = 0;
        }
        regGroup[pc] = 1024;
        BlackoutDevice();
        EnergizeDevice();
    }else if(orderType==0){//空指令
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==8){//出入栈
        uint32_t rw = ToolFunction::getBitsData(order,26,26);//出入栈
        uint32_t size = ToolFunction::getBitsData(order,25,24);//字节数
        uint32_t reg = ToolFunction::getBitsData(order,19,16);//出入栈的寄存器
        uint32_t num = ToolFunction::getBitsData(order,15,0);//入栈的立即数

        RwDatas sizeType;
        uint32_t sp_off;
        if(size==1){
            sizeType = RW_BYTE;
            sp_off = 1;
        }else if(size==2){
            sizeType =RW_WORD;
            sp_off = 2;
        }else if(size==3){
            sizeType = RW_DWORD;
            sp_off = 4;
        }else{
            regGroup[pc] = regGroup[pc]+4;
            return;
        }

        if(rw){//入栈
            if(reg!=0){
                num = regGroup[reg-1];
            }
            regGroup[sp] -= sp_off;
            if(isVirtualMem){
                WriteVirtualMem(num,regGroup[sp],sizeType);
            }else{
                WriteDeviceFun(num,regGroup[sp],sizeType);
            }
            regGroup[pc] = regGroup[pc]+4;
        }else{//出栈
            if(reg==0){
                regGroup[pc] = regGroup[pc]+4;
                return;
            }

            if(isVirtualMem){
                ReadVirtualMem(regGroup[reg-1],regGroup[sp],sizeType);
            }else{
                regGroup[reg-1] = ReadDeviceFun(regGroup[sp],sizeType);
            }
            regGroup[sp] += sp_off;
            regGroup[pc] = regGroup[pc]+4;
        }
    }else if(orderType==7){//读写内存
        uint32_t rw = ToolFunction::getBitsData(order,26,26);//读还是写
        uint32_t size = ToolFunction::getBitsData(order,25,24);//字节数
        uint32_t dataReg = ToolFunction::getBitsData(order,23,20);//内存地址的寄存器
        uint32_t addReg = ToolFunction::getBitsData(order,19,16);//读写的寄存器
        uint32_t num = ToolFunction::getBitsData(order,15,0);//立即数
        RwDatas sizeType;
        if(size==1){
            sizeType = RW_BYTE;
        }else if(size==2){
            sizeType =RW_WORD;
        }else if(size==3){
            sizeType = RW_DWORD;
        }else{
            regGroup[pc] = regGroup[pc]+4;
            return;
        }

        if(dataReg==0 || addReg==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        if(addReg!=0){
            num = regGroup[addReg-1];
        }

        if(rw){//写内存
            if(isVirtualMem){
                WriteVirtualMem(regGroup[dataReg-1],num,sizeType);
            }else{
                WriteDeviceFun(regGroup[dataReg-1],num,sizeType);
            }
            regGroup[pc] = regGroup[pc]+4;
        }else{//读内存

            if(isVirtualMem){
                ReadVirtualMem(regGroup[dataReg-1],num,sizeType);
            }else{
                regGroup[dataReg-1] = ReadDeviceFun(num,sizeType);
            }
            regGroup[pc] = regGroup[pc]+4;
        }
    }else if(orderType==16){//读栈内存
        uint32_t size = ToolFunction::getBitsData(order,26,25);//字节数
        uint32_t dataReg = ToolFunction::getBitsData(order,24,21);//读写的寄存器
        uint32_t addNum = ToolFunction::getBitsData(order,20,0) + regGroup[sp];//内存栈地址偏移
        RwDatas sizeType;
        if(size==1){
            sizeType = RW_BYTE;
        }else if(size==2){
            sizeType =RW_WORD;
        }else if(size==3){
            sizeType = RW_DWORD;
        }else{
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        if(dataReg==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        if(isVirtualMem){
            WriteVirtualMem(regGroup[dataReg-1],addNum,sizeType);
        }else{
            WriteDeviceFun(regGroup[dataReg-1],addNum,sizeType);
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==17){//写栈内存
        uint32_t size = ToolFunction::getBitsData(order,26,25);//字节数
        uint32_t dataReg = ToolFunction::getBitsData(order,24,21);//读写的寄存器
        uint32_t addNum = ToolFunction::getBitsData(order,20,0) + regGroup[sp];//内存栈地址偏移
        RwDatas sizeType;
        if(size==1){
            sizeType = RW_BYTE;
        }else if(size==2){
            sizeType =RW_WORD;
        }else if(size==3){
            sizeType = RW_DWORD;
        }else{
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        if(dataReg==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        if(isVirtualMem){
            ReadVirtualMem(regGroup[dataReg-1],addNum,sizeType);
        }else{
            regGroup[dataReg-1] = ReadDeviceFun(addNum,sizeType);
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==1 || orderType==2){//无符号四则运算-//有符号四则运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t regR = ToolFunction::getBitsData(order,15,12);
        uint32_t num = ToolFunction::getBitsData(order,11,0);

        if(regA==0 || regR==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }

        uint32_t aNum = regGroup[regA-1];
        uint32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }

        if(subType==0){
            regGroup[regR-1] = aNum+bNum;
        }else if(subType==1){
            regGroup[regR-1] = aNum-bNum;
        }else if(subType==2){
            regGroup[regR-1] = aNum*bNum;
        }else if(subType==3){
            regGroup[regR-1] = aNum/bNum;
            regGroup[flag] = aNum%bNum;
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==3){//浮点四则运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t regR = ToolFunction::getBitsData(order,15,12);
        uint32_t num = ToolFunction::getBitsData(order,11,0);

        if(regA==0 || regR==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }

        float aNum = DWORD(regGroup[regA-1]).toFloat();
        float bNum = DWORD(num).toFloat();
        if(regB!=0){
            bNum = DWORD(regGroup[regB-1]).toFloat();
        }

        if(subType==0){
            regGroup[regR-1] = DWORD(aNum+bNum).toUInt();
        }else if(subType==1){
            regGroup[regR-1] = DWORD(aNum-bNum).toUInt();
        }else if(subType==2){
            regGroup[regR-1] = DWORD(aNum*bNum).toUInt();
        }else if(subType==3){
            regGroup[regR-1] = DWORD(aNum/bNum).toUInt();
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==4){//浮整转换
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t num = ToolFunction::getBitsData(order,16,0);
        if(regA==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        uint32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }

        if(subType==0){
            //uint转float
            regGroup[regA-1] = DWORD((float)bNum).toUInt();
        }else if(subType==1){
            //int转float
            regGroup[regA-1] = DWORD((float)(int32_t)bNum).toUInt();
        }else if(subType==2){
            //float转int
            regGroup[regA-1] = (int32_t)DWORD(bNum).toFloat();
        }else if(subType==3){
            //-int
            regGroup[regA-1] = -((int32_t)bNum);
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==5){//移位运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t regR = ToolFunction::getBitsData(order,15,12);
        uint32_t num = ToolFunction::getBitsData(order,11,0);

        if(regA==0 || regR==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }

        uint32_t aNum = regGroup[regA-1];
        uint32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }
        if(bNum>=32){
            bNum = 32;
        }

        if(subType==0){
            if(bNum==0){
                regGroup[regR-1] = aNum;
                regGroup[flag] = 0;
            }else{
                regGroup[flag] = ToolFunction::getBitsData(aNum,31,32-bNum);
                regGroup[regR-1] = aNum<<bNum;
            }
        }else if(subType==1){
            if(bNum==0){
                regGroup[regR-1] = aNum;
                regGroup[flag] = 0;
            }else{
                regGroup[flag] = ToolFunction::getBitsData(aNum,bNum-1,0)<<(32-bNum);
                regGroup[regR-1] = aNum>>bNum;
            }
        }else if(subType==2){
            if(bNum==0){
                regGroup[regR-1] = aNum;
                regGroup[flag] = 0;
            }else{
                regGroup[flag] = ToolFunction::getBitsData(aNum,31,32-bNum);
                regGroup[regR-1] = (aNum<<bNum) | regGroup[flag];
            }
        }else if(subType==3){
            if(bNum==0){
                regGroup[regR-1] = aNum;
                regGroup[flag] = 0;
            }else{
                regGroup[flag] = ToolFunction::getBitsData(aNum,bNum-1,0)<<(32-bNum);
                regGroup[regR-1] = (aNum>>bNum) | regGroup[flag];
            }

        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==6){//位运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t regR = ToolFunction::getBitsData(order,15,12);
        uint32_t num = ToolFunction::getBitsData(order,11,0);

        uint32_t aNum = regGroup[regA-1];
        uint32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }

        if(subType==0){
            regGroup[regR-1] = aNum & bNum;
            regGroup[flag] = aNum && bNum;
        }else if(subType==1){
            regGroup[regR-1] = aNum | bNum;
            regGroup[flag] = aNum || bNum;
        }else if(subType==2){
            regGroup[regR-1] = ~bNum;
            regGroup[flag] = !(bool)bNum;
        }else if(subType==3){
            regGroup[regR-1] = aNum ^ bNum;
            regGroup[flag] = ((bool)aNum ^ (bool)bNum);
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==9){//寄存器数据转移/初始化

        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t num = ToolFunction::getBitsData(order,15,0);
        if(regA==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        uint32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }

        if(subType==0){
            regGroup[regA-1] = bNum;
        }else if(subType==1){
            regGroup[regA-1] = regGroup[regA-1]<<16 | bNum;
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==20){//无符号整型比较运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t num = ToolFunction::getBitsData(order,16,0);

        if(regA==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        uint32_t aNum = regGroup[regA-1];
        uint32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }

        if(subType==0){
            regGroup[flag] = aNum == bNum;
        }else if(subType==1){
            regGroup[flag] = aNum != bNum;
        }else if(subType==2){
            regGroup[flag] = aNum > bNum;
        }else if(subType==3){
            regGroup[flag] = aNum < bNum;
        }else if(subType==4){
            regGroup[flag] = aNum >= bNum;
        }else if(subType==5){
            regGroup[flag] = aNum <= bNum;
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==21){//有符号整型比较运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t num = ToolFunction::getBitsData(order,16,0);

        if(regA==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        int32_t aNum = regGroup[regA-1];
        int32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }

        if(subType==0){
            regGroup[flag] = aNum == bNum;
        }else if(subType==1){
            regGroup[flag] = aNum != bNum;
        }else if(subType==2){
            regGroup[flag] = aNum > bNum;
        }else if(subType==3){
            regGroup[flag] = aNum < bNum;
        }else if(subType==4){
            regGroup[flag] = aNum >= bNum;
        }else if(subType==5){
            regGroup[flag] = aNum <= bNum;
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==22){//浮点比较运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        float num = DWORD(ToolFunction::getBitsData(order,16,0)).toFloat();

        if(regA==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        float aNum = DWORD(regGroup[regA-1]).toFloat();
        float bNum = num;
        if(regB!=0){
            num = DWORD(regGroup[regB-1]).toFloat();
        }
        if(subType==0){
            regGroup[flag] = aNum == bNum;
        }else if(subType==1){
            regGroup[flag] = aNum != bNum;
        }else if(subType==2){
            regGroup[flag] = aNum > bNum;
        }else if(subType==3){
            regGroup[flag] = aNum < bNum;
        }else if(subType==4){
            regGroup[flag] = aNum >= bNum;
        }else if(subType==5){
            regGroup[flag] = aNum <= bNum;
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==18){//短有符号整型转32位标准有符号整型
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t num = ToolFunction::getBitsData(order,16,0);
        if(regA==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        int32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }

        if(subType==0){
            regGroup[regA-1] = (int)(char)bNum;
        }else if(subType==1){
            regGroup[regA-1] = (int)(short)bNum;
        }
        regGroup[pc] = regGroup[pc]+4;
    }else if(orderType==19){//bit位读取写入运算
        uint32_t subType = ToolFunction::getBitsData(order,26,24);
        uint32_t regA = ToolFunction::getBitsData(order,23,20);
        uint32_t regB = ToolFunction::getBitsData(order,19,16);
        uint32_t num = ToolFunction::getBitsData(order,16,10);
        uint32_t high = ToolFunction::getBitsData(order,9,5);
        uint32_t low = ToolFunction::getBitsData(order,4,0);
        if(regA==0){
            regGroup[pc] = regGroup[pc]+4;
            return;
        }
        int32_t bNum = num;
        if(regB!=0){
            bNum = regGroup[regB-1];
        }


        if(subType==0){
            uint32_t value = 0;
            if(high!=31){
                value |= ToolFunction::getBitsData(regGroup[regA-1],31,high+1)<<(high+1);
            }
            if(low!=0){
                value |= ToolFunction::getBitsData(regGroup[regA-1],low-1,0);
            }
            value |= ToolFunction::getBitsData(bNum,(high-low),0);
            regGroup[regA-1] = value;
        }else if(subType==1){
            regGroup[regA-1] = ToolFunction::getBitsData(bNum,high,low);
        }
        regGroup[pc] = regGroup[pc]+4;
    }else{
        if(isInter){
            regGroup[ipc] = regGroup[pc]+4;
            int num = 3;
            regGroup[pc] = ReadDeviceFun(num*4,RW_DWORD);
        }else{
            regGroup[pc] = regGroup[pc]+4;
        }
    }
}
QThread *cpuRunThread;//cpu运行线程
//读写寄存器数据
void VritualCPU::setRegData(uint32_t d,int regIndex){
    if(regIndex>=1 && regIndex<=15){
        regGroup[regIndex-1] = d;
    }
}
uint32_t VritualCPU::getRegData(int regIndex){
    if(regIndex>=1 && regIndex<=15){
        return regGroup[regIndex-1];
    }
    return -1;
}

