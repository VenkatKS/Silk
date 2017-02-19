//
//  main.c
//  FileSystemTestBench
//
//  Created by Venkat Srinivasan on 2/19/17.
//  Copyright © 2017 Venkat Srinivasan. All rights reserved.
//
#include <stdlib.h>
#include <stdio.h>
#include "OS_FileSystemScheme.h"
#include "Shell.h"

int main()
{
    OSFS_Init();
    Interpreter();
    return 0;
}
