﻿#include "pch.h"
#include "FacebookClient.h"
#include "FacebookSession.h"
#include "JsonClassFactory.h"
#include "SDKMessage.h"

using namespace winrt;
using namespace concurrency;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Web::Http;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage::Streams;
using namespace Windows::System::Threading;
using namespace Windows::Data::Json;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Web::Http::Headers;

#define BufferSize (1024 * 4) // 4kb
#define AttachmentMustHavePropertiesSetError L"Attachment (FBMediaObject/FBMediaStream) must have a content type, file name, and value set."
#define AttachmentValueIsNull L"The value of attachment (FBMediaObject/FBMediaStream) is null."
#define UnknownResponse = L"Unknown facebook response."
#define ETagKey L"_etag_"
#define MultiPartFormPrefix L"--"
#define MultiPartNewLine L"\r\n"
#define MultiPartContentType L"Content-Type: multipart/form-data; "
#define MultiPartBoundary L"------------------------------fbsdk1234567890"
#define UserAgent L"User-Agent"
#define WinSDKFBUserAgent(version) L"FBWinSDK." version
#define WinSDKFBUserAgentString WinSDKFBUserAgent(WINSDKFB_VERSION)

namespace winrt::FacebookSDK::implementation
{
	PropertySet FacebookClient::ToDictionary(
		PropertySet const& parameters,
		PropertySet const& mediaObjects,
		PropertySet const& mediaStreams
	) {
		if (parameters == nullptr)
		{
			return nullptr;
		}

		// Create a PropertySet to hold all objects that are not a MediaStream or mediaObject
		PropertySet dictionary;

		// Enumerate through all the parameters
		for (auto const& current : parameters)
		{
			hstring key(current.Key());
			if (current.Value().try_as<FacebookMediaObject>()) {
				mediaObjects.Insert(key, current.Value());
			}
			else if (current.Value().try_as<FacebookMediaStream>()) {
				mediaStreams.Insert(key, current.Value());
			}
			else {
				dictionary.Insert(key, current.Value());
			}
		}

		return dictionary;
	}

	IAsyncOperation<hstring> FacebookClient::GetTaskAsync(hstring const path, IMapView<hstring, IInspectable> const parameters)
	{
		PropertySet modifiableParams = MapViewToPropertySet(parameters);
		Uri uri = FacebookClient::PrepareRequestUri(path, modifiableParams);

		auto response = co_await FacebookClient::GetTaskInternalAsync(uri);

		if (FacebookClient::IsOAuthErrorResponse(response)) {
			auto sess = FacebookSession::ActiveSession();
			co_await sess.TryRefreshAccessTokenAsync();
			response = co_await FacebookClient::GetTaskInternalAsync(uri);
		}
		co_return response;
	}

	IAsyncOperation<hstring> FacebookClient::GetTaskInternalAsync(
		Uri const& RequestUri
	) {
		HttpBaseProtocolFilter filter;
		HttpClient httpClient(filter);
		httpClient.DefaultRequestHeaders().Append(UserAgent, WinSDKFBUserAgentString);

		filter.CacheControl().ReadBehavior(HttpCacheReadBehavior::Default);
		hstring result;
		try {
			auto response = co_await httpClient.GetAsync(RequestUri);
			result = co_await TryReceiveHttpResponse(response);
		}
		catch (hresult_error e) {
			OutputDebugString(e.message().data());
		}
		co_return result;
	}

	IAsyncOperation<hstring> FacebookClient::TryReceiveHttpResponse(
		HttpResponseMessage const& responseMessage)
	{
		hstring result;
		try
		{
			if (responseMessage && responseMessage.IsSuccessStatusCode())
			{
				result = co_await responseMessage.Content().ReadAsStringAsync();
			}
		}
		catch (hresult_error e)
		{
			OutputDebugString(e.message().data());
		}
		co_return result;
	}

	IAsyncOperation<hstring> FacebookClient::PostTaskAsync(hstring const path, IMapView<hstring, IInspectable> const parameters)
	{
		PropertySet modifiableParams = MapViewToPropertySet(parameters);
		Uri uri = FacebookClient::PrepareRequestUri(path, modifiableParams);
		hstring result;

		PropertySet streams = GetStreamsToUpload(modifiableParams);
		if (streams) {
			result = co_await FacebookClient::MultipartPostAsync(path, modifiableParams, streams);
		}
		else {
			result = co_await FacebookClient::SimplePostAsync(path, modifiableParams);
		}
		co_return result;
	}

	IAsyncOperation<hstring> FacebookClient::DeleteTaskAsync(hstring const path, IMapView<hstring, IInspectable> const parameters)
	{
		PropertySet modifiableParams = MapViewToPropertySet(parameters);
		Uri uri = FacebookClient::PrepareRequestUri(path, modifiableParams);

		hstring result;
		hstring response = co_await FacebookClient::DeleteTaskInternalAsync(uri);

		if (FacebookClient::IsOAuthErrorResponse(response)) {
			auto sess = FacebookSession::ActiveSession();
			co_await sess.TryRefreshAccessTokenAsync();
			result = co_await FacebookClient::DeleteTaskInternalAsync(uri);
		}
		else {
			result = response;
		}

		co_return result;
	}

	hstring FacebookClient::ParametersToQueryString(IMapView<hstring, IInspectable> const& parameters)
	{
		hstring queryString;

		// Add remaining parameters to query string.  Note that parameters that
		// do not need to be uploaded as multipart, i.e. any which is are not
		// binary data, are required to be in the query string, even for POST
		// requests!
		for (auto const& current : parameters)
		{
			auto key = Uri::EscapeComponent(current.Key());
			auto value = Uri::EscapeComponent(unbox_value<hstring>(current.Value()));

			if (queryString.size() > 0)
			{
				queryString = queryString + L"&";
			}

			queryString = queryString + key + L"=" + value;
		}

		return queryString;
	}

	/**
	 * Finds all FBMediaStream object in parameters.
	 * @param parameters The PropertySet to search for FBMediaStream objects in
	 * @return PropertySet containing all FBMediaStream objects found. If
	 * none are found, nullptr is instead returned.
	 */
	PropertySet FacebookClient::GetStreamsToUpload(PropertySet const& parameters)
	{
		PropertySet streams = nullptr;

		// Enumerate through all the parameters
		for (auto const& current : parameters)
		{
			if (streams.try_as<FacebookMediaStream>() != nullptr) {
				if (!streams) {
					streams = PropertySet();
				}
				streams.Insert(current.Key(), current.Value());
			}
		}

		return streams;
	}

	/**
	 * Adds FBMediaStream objects to Form.
	 * @param Parameters PropertySet that contains FBMediaStream objects to
	 * attach to Form. Will skip any values in Parameters that cannot be
	 * cast to FBMediaStream.
	 * @param Form The form to attach FBMediaStream objects to.
	 */
	void FacebookClient::AddStreamsToForm(PropertySet const& Parameters, HttpMultipartFormDataContent const& Form) {
		HttpStreamContent fileContent = nullptr;
		// Enumerate through all the parameters
		for (auto const& current : Parameters) {
			hstring key(current.Key());
			FacebookMediaStream stream = current.Value().as<FacebookMediaStream>();
			if (stream) {
				fileContent = HttpStreamContent(stream.Stream());
				HttpContentHeaderCollection headers(fileContent.Headers());
				headers.Insert(L"Content-Type", stream.Stream().ContentType());
				Form.Add(fileContent, key, stream.FileName());
			}
		}
	}

	/**
	 * Non-stream HTTP POST flow.
	 * @param path URL to send POST request to
	 * @param parameters query parameters to attach to POST request
	 * @return IAsyncOperation containing the response content
	 * @exception Can throw any exception that is thrown by SimplePlostInternalAsync
	 */
	IAsyncOperation<hstring> FacebookClient::SimplePostAsync(hstring const& path, PropertySet const& parameters) {
		Uri uri(FacebookClient::PrepareRequestUri(path, parameters));

		hstring result;
		hstring response = co_await FacebookClient::SimplePostInternalAsync(uri);
		if (FacebookClient::IsOAuthErrorResponse(response)) {
			//FBSession^ sess = FBSession::ActiveSession;
			//return FBSession::ActiveSession->TryRefreshAccessToken();
			result = co_await FacebookClient::SimplePostInternalAsync(uri);
		}
		else {
			result = response;
		}

		co_return result;
	}

	/**
	 * HTTP POST request flow with streams.
	 * @param path URL to send POST request to
	 * @param parameters query parameters to attach to POST request
	 * @param streams streams to attach to POST request
	 * @return IASyncOperation containing the response content
	 * @exception Can throw any exception that is thrown by MultipartPostInternalAsync
	 */
	IAsyncOperation<hstring> FacebookClient::MultipartPostAsync(
		hstring const& path,
		PropertySet const& streams,
		PropertySet const& parameters)
	{
		Uri uri(FacebookClient::PrepareRequestUri(path, parameters));

		hstring result;
		hstring response = co_await FacebookClient::MultipartPostInternalAsync(uri, streams);
		if (FacebookClient::IsOAuthErrorResponse(response)) {
			auto sess = FacebookSession::ActiveSession();
			co_await sess.TryRefreshAccessTokenAsync();
			result = co_await FacebookClient::MultipartPostInternalAsync(uri, streams);
		}
		else {
			result = response;
		}

		co_return result;
	}


	/**
	 * Builds request URI.
	 * @param httpMethod Type of HTTP request to build URI for
	 * @param path Request path
	 * @param parameters Query parameters for the request
	 * @return Request URI
	 * @exception FailureExecption if FBMediaObject or FBMediaStream are
	 * attempting to be attached on non-POST requests.
	 * @exception InvalidArgumentException if httpMethod is POST and improperly formatted/empty media object is attached.
	 */
	Uri FacebookClient::PrepareRequestUri(
		hstring const& path,
		PropertySet parameters
	) {
		auto sess = FacebookSession::ActiveSession();
		GraphUriBuilder uriBuilder(path);

		if (parameters == nullptr)
		{
			parameters = PropertySet();
		}

		PropertySet mediaObjects;
		PropertySet mediaStreams;
		PropertySet parametersWithoutMediaObjects = ToDictionary(parameters, mediaObjects, mediaStreams);
		// ensure that media items are in valid states
		ValidateMediaStreams(mediaStreams);
		ValidateMediaObjects(mediaObjects);

		if (parametersWithoutMediaObjects == nullptr)
		{
			parametersWithoutMediaObjects = PropertySet();
		}

		if (!parametersWithoutMediaObjects.HasKey(L"access_token") &&
			(sess.AccessTokenData() != nullptr) &&
			(sess.AccessTokenData().AccessToken().data() != nullptr) &&
			(sess.AccessTokenData().AccessToken().size() > 0))
		{
			parametersWithoutMediaObjects.Insert(L"access_token", box_value(sess.AccessTokenData().AccessToken()));
		}

		if (parametersWithoutMediaObjects.HasKey(L"format"))
		{
			parametersWithoutMediaObjects.Insert(L"format", box_value(L"json-strings"));
		}

		SerializeParameters(parametersWithoutMediaObjects);

		// Add remaining parameters to query string.  Note that parameters that
		// do not need to be uploaded as multipart, i.e. any which is are not
		// binary data, are required to be in the query string, even for POST
		// requests!
		auto kvp = parametersWithoutMediaObjects.First();
		while (kvp.HasCurrent())
		{
			uriBuilder.AddQueryParam(kvp.Current().Key(), unbox_value<hstring>(kvp.Current().Value()));
			kvp.MoveNext();
		}

		return uriBuilder.MakeUri();
	}

	/**
	 * Serializes non-string values in parameters to JSON strings. Note that
	 * this function modifies parameters.
	 * @param parameters The PropertySet to modify
	 */
	void FacebookClient::SerializeParameters(PropertySet const& parameters)
	{
		IVector<hstring> keysThatAreNotString = { winrt::single_threaded_vector<hstring>() };

		for (auto const& current : parameters)
		{
			if (winrt::unbox_value_or<hstring>(current.Value(), L"") == L"") {
				keysThatAreNotString.Append(current.Key());
			}
		}

		for (auto const& current : keysThatAreNotString)
		{
			auto val = parameters.Lookup(current);
			hstring newValue = unbox_value<hstring>(val);

			parameters.Remove(current);
			parameters.Insert(current, box_value(newValue));
		}
	}

	/**
	 * Checks if Response is an OAuth error response.
	 * @param Response response to check
	 * @return true if Response does indicate an OAuth error, false otherwise.
	 */
	bool FacebookClient::IsOAuthErrorResponse(hstring const& response)
	{
		auto err = FacebookError::FromJson(response);
		return (err && err.Code() == 190);
	}


	/**
	 * Performs the actual HTTP DELETE request.
	 * @param RequestUri the full URI of the request
	 * @return The response content
	 * @exception Exception Any exception that can occur during the request
	 */
	IAsyncOperation<hstring> FacebookClient::DeleteTaskInternalAsync(Uri const& RequestUri)
	{
		HttpBaseProtocolFilter filter;
		HttpClient httpClient(filter);
		httpClient.DefaultRequestHeaders().Append(UserAgent, WinSDKFBUserAgentString);

		auto response = co_await httpClient.DeleteAsync(RequestUri);
		auto result = co_await TryReceiveHttpResponse(response);
		co_return result;
	}

	/**
	 * Performs the actual HTTP POST request with no POST data.
	 * @param RequestUri the full URI of the request
	 * @return The response content
	 * @exception Exception Any exception that can occur during the request
	 */
	IAsyncOperation<hstring> FacebookClient::SimplePostInternalAsync(Uri const& RequestUri)
	{
		HttpBaseProtocolFilter filter;
		HttpClient httpClient(filter);
		httpClient.DefaultRequestHeaders().Append(UserAgent, WinSDKFBUserAgentString);
		OutputDebugString(RequestUri.AbsoluteUri().c_str());
		auto response = co_await httpClient.PostAsync(RequestUri, HttpStringContent(L""));
		auto result = co_await TryReceiveHttpResponse(response);
		co_return result;
	}

	/**
	 * Performs the actual HTTP POST request with POST stream data.
	 * @param RequestUri the full URI of the request
	 * @return The response content
	 * @exception Exception Any exception that can occur during the request
	 */
	IAsyncOperation<hstring> FacebookClient::MultipartPostInternalAsync(
		Uri const& RequestUri,
		PropertySet const& Streams
	) {
		HttpClient httpClient;
		httpClient.DefaultRequestHeaders().Append(UserAgent, WinSDKFBUserAgentString);
		HttpMultipartFormDataContent form;

		FacebookClient::AddStreamsToForm(Streams, form);

		auto response = co_await httpClient.PostAsync(RequestUri, form);
		auto result = co_await TryReceiveHttpResponse(response);
		co_return result;
	}

	PropertySet FacebookClient::MapViewToPropertySet(IMapView<hstring, IInspectable> const& mapView)
	{
		PropertySet propertySet;
		for (auto const & current : mapView)
		{
			propertySet.Insert(current.Key(), current.Value());
		}
		return propertySet;
	}

	void FacebookClient::ValidateMediaStreams(PropertySet const& mediaStreams)
	{
		if (mediaStreams.Size() > 0)
		{
			for (auto const& current : mediaStreams)
			{
				FacebookMediaStream mediaStream = unbox_value<FacebookMediaStream>(current.Value());

				IRandomAccessStream stream(mediaStream.Stream());
				if (stream == nullptr)
				{
					throw hresult_invalid_argument(AttachmentValueIsNull);
				}

				if ((mediaStream.Stream() == nullptr) ||
					(mediaStream.Stream().ContentType().empty()) ||
					(mediaStream.FileName().empty()))
				{
					throw hresult_invalid_argument(AttachmentMustHavePropertiesSetError);
				}

			}
		}
	}

	void FacebookClient::ValidateMediaObjects(PropertySet const& mediaObjects)
	{
		if (mediaObjects.Size() > 0)
		{
			for (auto const& current : mediaObjects)
			{
				FacebookMediaObject mediaObject = unbox_value<FacebookMediaObject>(current.Value());

				if (mediaObject.GetValue().empty())
				{
					throw hresult_invalid_argument(AttachmentValueIsNull);
				}

				if ((mediaObject.GetValue().empty()) ||
					(mediaObject.ContentType().empty()) ||
					(mediaObject.FileName().empty()))
				{
					throw hresult_invalid_argument(AttachmentMustHavePropertiesSetError);
				}

			}
		}
	}
}
