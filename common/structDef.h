/*---------------------------------------------------------------------------------
* Copyright(C),2018
* FileName:  strcutDef.h定义
* Author:    luoyouchun
* Version:   1.0
* Date:      2018-8-111
* Description:  报文收发结果
*---------------------------------------------------------------------------------
* Modification Histroy
Date    Author      Modification:
*
*---------------------------------------------------------------------------------*/

#pragma once

#ifndef _STRUCTDEF_H_
#define _STRUCTDEF_H_

namespace lpp
{
    /// <summary>
    /// 报文发送消息结果定义
    /// </summary>
    enum class eStatus
    {
        sSucceed = 1,
        sFailed,
        sTimeout,
    };

}

#endif  // _STRUCTDEF_H_
