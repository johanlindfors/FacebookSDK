﻿#pragma once

#include "FacebookSession.g.h"
#include <ppltasks.h>

namespace winrt::FacebookSDK::implementation
{
	struct FacebookSession : FacebookSessionT<FacebookSession>
	{
		FacebookSession();

		hstring FacebookAppId();
		void FacebookAppId(hstring const& value);
		hstring WinAppId();
		void WinAppId(hstring const& value);
		hstring AppResponse();
		bool LoggedIn();
		FacebookSDK::FacebookAccessTokenData AccessTokenData();
		void AccessTokenData(FacebookSDK::FacebookAccessTokenData const& value);
		int32_t APIMajorVersion();
		int32_t APIMinorVersion();
		FacebookSDK::Graph::FBUser User();
		hstring WebViewRedirectDomain();
		hstring WebViewRedirectPath();
		Windows::Foundation::IAsyncAction LogoutAsync();
		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> ShowFeedDialogAsync(Windows::Foundation::Collections::PropertySet const Parameters);
		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> ShowRequestsDialogAsync(Windows::Foundation::Collections::PropertySet const Parameters);
		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> ShowSendDialogAsync(Windows::Foundation::Collections::PropertySet const Parameters);
		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> LoginAsync();
		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> LoginAsync(FacebookSDK::FacebookPermissions const permissions);
		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> LoginAsync(FacebookSDK::FacebookPermissions const permissions, FacebookSDK::SessionLoginBehavior const behavior);
		void SetApiVersion(int32_t major, int32_t minor);
		void SetWebViewRedirectUrl(hstring const& domain, hstring const& Path);

		static FacebookSDK::FacebookSession ActiveSession();
		static Windows::Storage::ApplicationDataContainer DataContainer();

		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> TryRefreshAccessTokenAsync();

	private:

		~FacebookSession();

		Windows::Foundation::Uri BuildLoginUri(
			Windows::Foundation::Collections::PropertySet Parameters
		);

		hstring GetWebAuthRedirectUriString();

		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> GetUserInfo(
			FacebookSDK::FacebookAccessTokenData const& tokenData
		);

		void ParseOAuthResponse(Windows::Foundation::Uri ResponseUri);

		Windows::Foundation::IAsyncOperation<Windows::Storage::IStorageItem>
			MyTryGetItemAsync(
				Windows::Storage::StorageFolder folder,
				hstring itemName
			);

		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> CheckForExistingTokenAsync();

		winrt::fire_and_forget TrySaveTokenData();

		Windows::Foundation::IAsyncAction TryDeleteTokenData();

		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> GetAppPermissions();

		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult>
			ProcessAuthResult(
				Windows::Security::Authentication::Web::WebAuthenticationResult authResult
			);

		concurrency::task<FacebookSDK::FacebookResult> TryGetUserInfoAfterLogin(
			FacebookSDK::FacebookResult loginResult
		);

		concurrency::task<FacebookSDK::FacebookResult> TryGetAppPermissionsAfterLogin(
			FacebookSDK::FacebookResult loginResult
		);

		concurrency::task<FacebookSDK::FacebookResult> RunOAuthOnUiThread(
			Windows::Foundation::Collections::PropertySet Parameters
		);

		concurrency::task<FacebookSDK::FacebookResult> RunWebViewLoginOnUIThread(
			Windows::Foundation::Collections::PropertySet Parameters
		);

		Windows::Foundation::IAsyncOperation<FacebookSDK::FacebookResult> ShowLoginDialogAsync(
			Windows::Foundation::Collections::PropertySet const& Parameters
		);

		concurrency::task<FacebookSDK::FacebookResult> TryLoginViaWebView(
			Windows::Foundation::Collections::PropertySet Parameters
		);

		concurrency::task<FacebookSDK::FacebookResult> TryLoginViaWebAuthBroker(
			Windows::Foundation::Collections::PropertySet Parameters
		);

		concurrency::task<FacebookSDK::FacebookResult> TryLoginSilently(
			Windows::Foundation::Collections::PropertySet Parameters
		);

		void SaveGrantedPermissions();

		hstring GetGrantedPermissions();

#if defined(_WIN32_WINNT_WIN10) && (_WIN32_WINNT >= _WIN32_WINNT_WIN10)
		hstring GetWebAccountProviderRedirectUriString();

		concurrency::task<FacebookSDK::FacebookResult> CheckWebAccountProviderForExistingToken(
			FacebookSDK::FacebookPermissions Permissions
		);

		concurrency::task<FacebookSDK::FacebookResult> TryLoginViaWebAccountProvider(
			FacebookSDK::FacebookPermissions Permissions
		);

		concurrency::task<FacebookSDK::FacebookResult> CallWebAccountProviderOnUiThread(
			FacebookSDK::FacebookPermissions Permissions
		);

		FacebookSDK::FacebookResult ExtractAccessTokenDataFromResponseData(
			Windows::Foundation::Collections::IVectorView
			<Windows::Security::Authentication::Web::Core::WebTokenResponse> ResponseData
		);

		FacebookSDK::FacebookResult FBResultFromTokenRequestResult(
			Windows::Security::Authentication::Web::Core::WebTokenRequestResult RequestResult
		);
#endif

#if WINAPI_FAMILY==WINAPI_FAMILY_PHONE_APP
		Windows::Foundation::Ur^ RemoveJSONFromBrowserResponseUri(
			Windows::Foundation::Uri responseUri
		);
#endif

		BOOL IsRerequest(
			Windows::Foundation::Collections::PropertySet Parameters
		);

		bool _loggedIn;
		int _APIMajorVersion;
		int _APIMinorVersion;
		std::wstring _FBAppId;
		std::wstring _WinAppId;
		std::wstring _AppResponse;
		std::wstring _webViewRedirectDomain;
		std::wstring _webViewRedirectPath;
		FacebookSDK::FacebookAccessTokenData _AccessTokenData{ nullptr };
		FacebookSDK::Graph::FBUser _user{ nullptr };
		concurrency::task<FacebookSDK::FacebookResult> _loginTask;
		FacebookSDK::FacebookDialog _dialog{ nullptr };	
};
}

namespace winrt::FacebookSDK::factory_implementation
{
	struct FacebookSession : FacebookSessionT<FacebookSession, implementation::FacebookSession>
	{
	};
}
