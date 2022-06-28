//
//  ViewController.h
//  fiber_http
//
//  Created by shuxin 　　zheng on 2020/4/19.
//  Copyright © 2020 acl. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

@property (nonatomic, retain) IBOutlet UITextField *tfUrl;
- (IBAction) request:(id)obj;
@end

