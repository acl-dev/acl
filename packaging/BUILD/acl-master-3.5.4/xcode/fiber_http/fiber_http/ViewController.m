//
//  ViewController.m
//  fiber_http
//
//  Created by shuxin 　　zheng on 2020/4/19.
//  Copyright © 2020 acl. All rights reserved.
//

#import "ViewController.h"
#import "HttpRequest.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (IBAction)request:(id)obj {
    NSString *url = [_tfUrl text];
    http_request* req = [[http_request alloc] init];
    NSString* res = [req test: url];
    return;
    NSString *message = [NSString stringWithFormat:@"url：%@\r\n\r\n%@", url, res];
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Information" message:message delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
    [alert show];
}
@end
