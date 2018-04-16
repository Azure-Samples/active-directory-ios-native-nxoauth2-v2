#import <Foundation/Foundation.h>

@interface User : NSObject {
    NSString *upn;
    NSString *name;
}

@property (nonatomic, copy) NSString *upn;
@property (nonatomic, copy) NSString *name;
@property (nonatomic, copy) NSString *mail;
@property (nonatomic, copy) NSString *businessPhones;
@property (nonatomic, copy) NSString *mobilePhones;

+ (id)NameOfUpn:(NSString*)upn name:(NSString*)name mail:(NSString *)mail mobilePhones:(NSString *)mobile businessPhones:(NSString *)business;

@end
