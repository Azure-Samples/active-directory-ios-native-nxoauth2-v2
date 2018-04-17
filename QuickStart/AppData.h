//
//  AppData.h
//  QuickStart
//
//  Created by Brandon Werner on 6/1/15.
//  Copyright (c) 2015 Microsoft. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface AppData : NSObject

@property (strong) NSString* scopes;
@property (strong) NSString* graphApiUrlString;
@property (strong) NSString* authority;
@property (strong) NSString* clientId;
@property (strong) NSString* resourceId;
@property (strong) NSString* redirectUriString;
@property (strong) NSString* apiversion;
@property (strong) NSString* tenant;
@property (strong) NSString* secret;

+(id) getInstance;

@end
