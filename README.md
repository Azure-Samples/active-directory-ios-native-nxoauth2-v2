---
services: active-directory
platforms: ios
author: dadobali
---

**NOTE regarding iOS 9:**

Apple has released iOS 9 which includes support for App Transport Security (ATS). ATS restricts apps from accessing the internet unless they meet several security requirements incuding TLS 1.2 and SHA-256. While Microsoft's APIs support these standards some third party APIs and content delivery networks we use have yet to be upgraded. This means that any app that relies on Azure Active Directory or Microsoft Accounts will fail when compiled with iOS 9. For now our recommendation is to disable ATS, which reverts to iOS 8 functionality. Please refer to this [technote from Apple](https://developer.apple.com/library/prerelease/ios/technotes/App-Transport-Security-Technote/) for more informtaion.

----

Azure AD provides the Active Directory Authentication Library, or ADAL, for iOS clients that need to access protected resources.  ADAL’s sole purpose in life is to make it easy for your app to get access tokens.  To demonstrate just how easy it is, here we’ll build a Objective C To-Do List application that:

-	Gets access tokens for calling the Azure AD Graph API using the [OAuth 2.0 authentication protocol](https://msdn.microsoft.com/library/azure/dn645545.aspx).
-	Searches a directory for users with a given alias.

To build the complete working application, you’ll need to:

1. Register your application with Azure AD.
2. Install & Configure ADAL.
3. Use ADAL to get tokens from Azure AD.

To get started, [download the completed sample](https://github.com/AzureADQuickStarts/NativeClient-iOS/archive/complete.zip).  You'll also need an Azure AD tenant in which you can create users and register an application.  If you don't already have a tenant, [learn how to get one](active-directory-howto-tenant.md).

## *1. Determine what your Redirect URI will be for iOS*

In order to securely launch your applications in certain SSO scenarios we require that you create a **Redirect URI** in a particular format. A Redirect URI is used to ensure that the tokens return to the correct application that asked for them.

The iOS format for a Redirect URI is:

```
<app-scheme>://<bundle-id>
```

- 	**aap-scheme** - This is registered in your XCode project. It is how other applications can call you. You can find this under Info.plist -> URL types -> URL Identifier. You should create one if you don't already have one or more configured.
- 	**bundle-id** - This is the Bundle Identifier found under "identity" un your project settings in XCode.

An example for this QuickStart code would be: ***msquickstart://com.microsoft.azureactivedirectory.samples.graph.QuickStart***

## *2. Register the DirectorySearcher Application*
To enable your app to get tokens, you'll first need to register it in your Azure AD tenant and grant it permission to access the Azure AD Graph API:

1. Sign in to the [Azure portal](https://portal.azure.com).
2. On the top bar, click on your account and under the **Directory** list, choose the Active Directory tenant where you wish to register your application.
3. Click on **More Services** in the left hand nav, and choose **Azure Active Directory**.
4. Click on **App registrations** and choose **Add**.
5. Enter a friendly name for the application, for example 'DirectorySearcher' and select 'Native' as the Application Type. The **Redirect Uri** is a scheme and string combination that Azure AD will use to return token responses.  Enter a value specific to your application based on the information above. Click on **Create** to create the application.
6. While still in the Azure portal, choose your application, click on **Settings** and choose **Properties**.
7. Find the Application ID value and copy it to the clipboard.
8. Configure Permissions for your application - in the Settings menu, choose the 'Required permissions' section, click on **Add**, then **Select an API**, and select 'Microsoft Graph' (this is the Graph API). Then, click on  **Select Permissions** and select 'Read Directory Data'.


## *3. Install & Configure ADAL*
Now that you have an application in Azure AD, you can install ADAL and write your identity-related code.  In order for ADAL to be able to communicate with Azure AD, you need to provide it with some information about your app registration.
-	Begin by adding ADAL to the DirectorySearcher project using Cocapods.

```
$ vi Podfile
```
Add the following to this podfile:

```
source 'https://github.com/CocoaPods/Specs.git'
link_with ['QuickStart']
xcodeproj 'QuickStart'

pod 'ADALiOS'
```

Now load the podfile using cocoapods. This will create a new XCode Workspace you will load.

```
$ pod install
...
$ open QuickStart.xcworkspace
```

-	In the QuickStart project, open the plist file `settings.plist`.  Replace the values of the elements in the section to reflect the values you input into the Azure Portal.  Your code will reference these values whenever it uses ADAL.
    -	The `tenant` is the domain of your Azure AD tenant, e.g. contoso.onmicrosoft.com
    -	The `clientId` is the clientId of your application you copied from the portal.
    -	The `redirectUri` is the redirect url you registered in the portal.

## *4.	Use ADAL to Get Tokens from AAD*
The basic principle behind ADAL is that whenever your app needs an access token, it simply calls a completionBlock `+(void) getToken : `, and ADAL does the rest.  

-	In the `QuickStart` project, open `GraphAPICaller.m` and locate the `// TODO: getToken for generic Web API flows. Returns a token with no additional parameters provided.` comment near the top.  This is where you pass ADAL the coordinates through a CompletionBlock to communicate with Azure AD and tell it how to cache tokens.

```ObjC
+(void) getToken : (BOOL) clearCache
           parent:(UIViewController*) parent
completionHandler:(void (^) (NSString*, NSError*))completionBlock;
{
    AppData* data = [AppData getInstance];
    if(data.userItem){
        completionBlock(data.userItem.accessToken, nil);
        return;
    }

    ADAuthenticationError *error;
    authContext = [ADAuthenticationContext authenticationContextWithAuthority:data.authority error:&error];
    authContext.parentController = parent;
    NSURL *redirectUri = [[NSURL alloc]initWithString:data.redirectUriString];

    [ADAuthenticationSettings sharedInstance].enableFullScreen = YES;
    [authContext acquireTokenWithResource:data.resourceId
                                 clientId:data.clientId
                              redirectUri:redirectUri
                           promptBehavior:AD_PROMPT_AUTO
                                   userId:data.userItem.userInformation.userId
                     extraQueryParameters: @"nux=1" // if this strikes you as strange it was legacy to display the correct mobile UX. You most likely won't need it in your code.
                          completionBlock:^(ADAuthenticationResult *result) {

                              if (result.status != AD_SUCCEEDED)
                              {
                                  completionBlock(nil, result.error);
                              }
                              else
                              {
                                  data.userItem = result.tokenCacheStoreItem;
                                  completionBlock(result.tokenCacheStoreItem.accessToken, nil);
                              }
                          }];
}

```

- Now we need to use this token to search for users in the graph. Find the `// TODO: implement SearchUsersList` commentThis method makes a GET request to the Azure AD Graph API to query for users whose UPN begins with the given search term.  But in order to query the Graph API, you need to include an access_token in the `Authorization` header of the request - this is where ADAL comes in.

```ObjC
+(void) searchUserList:(NSString*)searchString
                parent:(UIViewController*) parent
       completionBlock:(void (^) (NSMutableArray* Users, NSError* error)) completionBlock
{
    if (!loadedApplicationSettings)
    {
        [self readApplicationSettings];
    }

    AppData* data = [AppData getInstance];

    NSString *graphURL = [NSString stringWithFormat:@"%@%@/users?api-version=%@&$filter=startswith(userPrincipalName, '%@')", data.taskWebApiUrlString, data.tenant, data.apiversion, searchString];


    [self craftRequest:[self.class trimString:graphURL]
                parent:parent
     completionHandler:^(NSMutableURLRequest *request, NSError *error) {

         if (error != nil)
         {
             completionBlock(nil, error);
         }
         else
         {

             NSOperationQueue *queue = [[NSOperationQueue alloc]init];

             [NSURLConnection sendAsynchronousRequest:request queue:queue completionHandler:^(NSURLResponse *response, NSData *data, NSError *error) {

                 if (error == nil && data != nil){

                     NSDictionary *dataReturned = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];

                     // We can grab the top most JSON node to get our graph data.
                     NSArray *graphDataArray = [dataReturned objectForKey:@"value"];

                     // Don't be thrown off by the key name being "value". It really is the name of the
                     // first node. :-)

                     //each object is a key value pair
                     NSDictionary *keyValuePairs;
                     NSMutableArray* Users = [[NSMutableArray alloc]init];

                     for(int i =0; i < graphDataArray.count; i++)
                     {
                         keyValuePairs = [graphDataArray objectAtIndex:i];

                         User *s = [[User alloc]init];
                         s.upn = [keyValuePairs valueForKey:@"userPrincipalName"];
                         s.name =[keyValuePairs valueForKey:@"givenName"];

                         [Users addObject:s];
                     }

                     completionBlock(Users, nil);
                 }
                 else
                 {
                     completionBlock(nil, error);
                 }

             }];
         }
     }];

}

```
- When your app requests a token by calling `getToken(...)`, ADAL will attempt to return a token without asking the user for credentials.  If ADAL determines that the user needs to sign in to get a token, it will display a login dialog, collect the user's credentials, and return a token upon successful authentication.  If ADAL is unable to return a token for any reason, it will throw an `AdalException`.
- Notice that the `AuthenticationResult` object contains a `tokenCacheStoreItem` object that can be used to collect information your app may need.  In the QuickStart, `tokenCacheStoreItem` is used to determine if authenitcation has already occurred.


## Step 5: Build and Run the application



Congratulations! You now have a working iOS application that has the ability to authenticate users, securely call Web APIs using OAuth 2.0, and get basic information about the user.  If you haven't already, now is the time to populate your tenant with some users.  Run your QuickStart app, and sign in with one of those users.  Search for other users based on their UPN.  Close the app, and re-run it.  Notice how the user's session remains intact.

ADAL makes it easy to incorporate all of these common identity features into your application.  It takes care of all the dirty work for you - cache management, OAuth protocol support, presenting the user with a login UI, refreshing expired tokens, and more.  All you really need to know is a single API call, `getToken`.

For reference, the completed sample (without your configuration values) is provided [here](https://github.com/AzureADQuickStarts/NativeClient-iOS/archive/complete.zip).  You can now move on to additional scenarios.  You may want to try:

[Secure a Node.JS Web API with Azure AD >>](active-directory-devquickstarts-webapi-nodejst.md)

For additional resources, check out:
- [AzureADSamples on GitHub >>](https://github.com/AzureAdSamples)
- [CloudIdentity.com >>](https://cloudidentity.com)
- Azure AD documentation on [Azure.com >>](http://azure.microsoft.com/documentation/services/active-directory/)
