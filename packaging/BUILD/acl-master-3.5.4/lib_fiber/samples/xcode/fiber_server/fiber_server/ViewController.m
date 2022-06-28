//
//  ViewController.m
//  fiber_server
//
//  Created by shuxin 　　zheng on 2020/9/25.
//  Copyright © 2020 acl. All rights reserved.
//

#import "ViewController.h"
#import "FiberTest.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
    FiberTest *test = [[FiberTest alloc] init];
    [test Start];
}


@end
