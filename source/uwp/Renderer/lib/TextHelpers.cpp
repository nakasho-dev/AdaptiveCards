#include "pch.h"
#include "AdaptiveTextRun.h"
#include "DateTimeParser.h"
#include "MarkDownParser.h"
#include "TextHelpers.h"
#include "XamlBuilder.h"
#include "XamlHelpers.h"
#include <safeint.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::AdaptiveNamespace;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace AdaptiveNamespace;
using namespace AdaptiveSharedNamespace;
using namespace msl::utilities;
using namespace ABI::Windows::UI::Xaml::Controls;
using namespace ABI::Windows::UI::Xaml::Media;
using namespace ABI::Windows::UI::Xaml;

HRESULT SetMaxLines(ITextBlock* textBlock, UINT maxLines)
{
    ComPtr<ITextBlock> localTextBlock(textBlock);
    ComPtr<ITextBlock2> xamlTextBlock2;
    localTextBlock.As(&xamlTextBlock2);
    RETURN_IF_FAILED(xamlTextBlock2->put_MaxLines(maxLines));
    return S_OK;
}

HRESULT SetMaxLines(IRichTextBlock* textBlock, UINT maxLines)
{
    ComPtr<IRichTextBlock> localTextBlock(textBlock);
    ComPtr<IRichTextBlock2> xamlTextBlock2;
    localTextBlock.As(&xamlTextBlock2);
    RETURN_IF_FAILED(xamlTextBlock2->put_MaxLines(maxLines));
    return S_OK;
}

HRESULT SetXamlInlinesWithTextConfig(_In_ IAdaptiveRenderContext* renderContext,
                                     _In_ IAdaptiveRenderArgs* renderArgs,
                                     _In_ IAdaptiveTextConfig* textConfig,
                                     _In_ HSTRING language,
                                     _In_ HSTRING text,
                                     _In_ ITextBlock* textBlock)
{
    // Create an AdaptiveTextRun with the language, text, and configuration to pass to SetXamlInlines
    ComPtr<AdaptiveNamespace::AdaptiveTextRun> textRun;
    RETURN_IF_FAILED(MakeAndInitialize<AdaptiveNamespace::AdaptiveTextRun>(&textRun));

    RETURN_IF_FAILED(textRun->put_Text(text));
    RETURN_IF_FAILED(textRun->put_Language(language));

    ABI::AdaptiveNamespace::TextWeight weight;
    RETURN_IF_FAILED(textConfig->get_Weight(&weight));
    RETURN_IF_FAILED(textRun->put_Weight(weight));

    ABI::AdaptiveNamespace::ForegroundColor color;
    RETURN_IF_FAILED(textConfig->get_Color(&color));
    RETURN_IF_FAILED(textRun->put_Color(color));

    ABI::AdaptiveNamespace::TextSize size;
    RETURN_IF_FAILED(textConfig->get_Size(&size));
    RETURN_IF_FAILED(textRun->put_Size(size));

    boolean isSubtle;
    RETURN_IF_FAILED(textConfig->get_IsSubtle(&isSubtle));
    RETURN_IF_FAILED(textRun->put_IsSubtle(isSubtle));

    ComPtr<IVector<ABI::Windows::UI::Xaml::Documents::Inline*>> inlines;
    RETURN_IF_FAILED(textBlock->get_Inlines(&inlines));

    // Style the text block with the properties from the TextRun
    RETURN_IF_FAILED(StyleTextElement(textRun.Get(), renderContext, renderArgs, false, textBlock));

    // Set the inlines
    RETURN_IF_FAILED(SetXamlInlines(textRun.Get(), renderContext, renderArgs, false, inlines.Get()));

    // Set wrap and maxwidth
    boolean wrap;
    RETURN_IF_FAILED(textConfig->get_Wrap(&wrap));
    RETURN_IF_FAILED(SetWrapProperties(textBlock, wrap, true));

    ComPtr<IFrameworkElement> textBlockAsFrameworkElement;
    ComPtr<ITextBlock> localTextBlock(textBlock);
    RETURN_IF_FAILED(localTextBlock.As(&textBlockAsFrameworkElement));

    UINT32 maxWidth;
    RETURN_IF_FAILED(textConfig->get_MaxWidth(&maxWidth));
    textBlockAsFrameworkElement->put_MaxWidth(maxWidth);

    return S_OK;
}

HRESULT SetXamlInlines(_In_ ABI::AdaptiveNamespace::IAdaptiveTextElement* adaptiveTextElement,
                       _In_ ABI::AdaptiveNamespace::IAdaptiveRenderContext* renderContext,
                       _In_ ABI::AdaptiveNamespace::IAdaptiveRenderArgs* renderArgs,
                       bool isInHyperlink,
                       _In_ IVector<ABI::Windows::UI::Xaml::Documents::Inline*>* inlines,
                       _Out_opt_ UINT* characterLength)
{
    HString text;
    RETURN_IF_FAILED(adaptiveTextElement->get_Text(text.GetAddressOf()));

    HString language;
    RETURN_IF_FAILED(adaptiveTextElement->get_Language(language.GetAddressOf()));

    DateTimeParser parser(HStringToUTF8(language.Get()));
    auto textWithParsedDates = parser.GenerateString(HStringToUTF8(text.Get()));

    MarkDownParser markdownParser(textWithParsedDates);
    auto htmlString = markdownParser.TransformToHtml();

    bool handledAsHtml = false;
    UINT localCharacterLength;
    if (markdownParser.HasHtmlTags())
    {
        HString htmlHString;
        UTF8ToHString(htmlString, htmlHString.GetAddressOf());

        ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocument> xmlDocument =
            XamlHelpers::CreateXamlClass<ABI::Windows::Data::Xml::Dom::IXmlDocument>(
                HStringReference(RuntimeClass_Windows_Data_Xml_Dom_XmlDocument));

        ComPtr<ABI::Windows::Data::Xml::Dom::IXmlDocumentIO> xmlDocumentIO;
        RETURN_IF_FAILED(xmlDocument.As(&xmlDocumentIO));

        HRESULT hr = xmlDocumentIO->LoadXml(htmlHString.Get());
        if (SUCCEEDED(hr))
        {
            ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> xmlDocumentAsNode;
            RETURN_IF_FAILED(xmlDocument.As(&xmlDocumentAsNode));

            RETURN_IF_FAILED(
                AddHtmlInlines(adaptiveTextElement, renderContext, renderArgs, xmlDocumentAsNode.Get(), isInHyperlink, inlines, &localCharacterLength));
            handledAsHtml = true;
        }
    }

    if (!handledAsHtml)
    {
        HString hString;
        UTF8ToHString(textWithParsedDates, hString.GetAddressOf());
        AddSingleTextInline(adaptiveTextElement, renderContext, renderArgs, hString.Get(), false, false, isInHyperlink, inlines, &localCharacterLength);
    }

    if (characterLength)
    {
        *characterLength = localCharacterLength;
    }

    return S_OK;
}

static HRESULT GetTextFromXmlNode(_In_ ABI::Windows::Data::Xml::Dom::IXmlNode* node, _In_ HSTRING* text)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> localNode = node;
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeSerializer> textNodeSerializer;
    RETURN_IF_FAILED(localNode.As(&textNodeSerializer));

    RETURN_IF_FAILED(textNodeSerializer->get_InnerText(text));
    return S_OK;
}

HRESULT AddListInlines(_In_ ABI::AdaptiveNamespace::IAdaptiveTextElement* adaptiveTextElement,
                       _In_ ABI::AdaptiveNamespace::IAdaptiveRenderContext* renderContext,
                       _In_ ABI::AdaptiveNamespace::IAdaptiveRenderArgs* renderArgs,
                       _In_ ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                       bool isListOrdered,
                       bool isInHyperlink,
                       _In_ IVector<ABI::Windows::UI::Xaml::Documents::Inline*>* inlines,
                       _Out_ UINT* characterLength)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap> attributeMap;
    RETURN_IF_FAILED(node->get_Attributes(&attributeMap));

    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> startNode;
    RETURN_IF_FAILED(attributeMap->GetNamedItem(HStringReference(L"start").Get(), &startNode));

    unsigned long iteration = 1;
    if (startNode != nullptr)
    {
        // Get the starting value for this list
        HString start;
        RETURN_IF_FAILED(GetTextFromXmlNode(startNode.Get(), start.GetAddressOf()));
        try
        {
            iteration = std::stoul(HStringToUTF8(start.Get()));

            // Check that we can iterate the entire list without overflowing.
            // If the list values are too big to store in an unsigned int, start
            // the list at 1.
            ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNodeList> children;
            RETURN_IF_FAILED(node->get_ChildNodes(&children));

            UINT32 childrenLength;
            RETURN_IF_FAILED(children->get_Length(&childrenLength));

            unsigned long result;
            if (!SafeAdd(iteration, childrenLength - 1, result))
            {
                iteration = 1;
            }
        }
        catch (const std::out_of_range&)
        {
            // If stoul throws out_of_range, start the numbered list at 1.
        }
    }

    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> listChild;
    RETURN_IF_FAILED(node->get_FirstChild(&listChild));

    UINT totalCharacterLength = 0;
    bool isFirstListElement = true;
    while (listChild != nullptr)
    {
        std::wstring listElementString = isFirstListElement ? L"" : L"\n";
        if (!isListOrdered)
        {
            // Add a bullet
            listElementString += L"\x2022 ";
        }
        else
        {
            listElementString += std::to_wstring(iteration);
            listElementString += L". ";
        }

        HString listElementHString;
        RETURN_IF_FAILED(listElementHString.Set(listElementString.c_str()));

        totalCharacterLength += WindowsGetStringLen(listElementHString.Get());

        ComPtr<ABI::Windows::UI::Xaml::Documents::IRun> run =
            XamlHelpers::CreateXamlClass<ABI::Windows::UI::Xaml::Documents::IRun>(
                HStringReference(RuntimeClass_Windows_UI_Xaml_Documents_Run));
        RETURN_IF_FAILED(run->put_Text(listElementHString.Get()));

        ComPtr<ABI::Windows::UI::Xaml::Documents::ITextElement> runAsTextElement;
        RETURN_IF_FAILED(run.As(&runAsTextElement));

        // Make sure the bullet or list number is styled correctly
        RETURN_IF_FAILED(
            StyleTextElement(adaptiveTextElement, renderContext, renderArgs, isInHyperlink, runAsTextElement.Get()));

        ComPtr<ABI::Windows::UI::Xaml::Documents::IInline> runAsInline;
        RETURN_IF_FAILED(run.As(&runAsInline));

        RETURN_IF_FAILED(inlines->Append(runAsInline.Get()));

        UINT textCharacterLength = 0;
        RETURN_IF_FAILED(
            AddTextInlines(adaptiveTextElement, renderContext, renderArgs, listChild.Get(), false, false, isInHyperlink, inlines, &textCharacterLength));
        totalCharacterLength += textCharacterLength;

        ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> nextListChild;
        RETURN_IF_FAILED(listChild->get_NextSibling(&nextListChild));

        iteration++;
        listChild = nextListChild;
        isFirstListElement = false;
    }

    *characterLength = totalCharacterLength;
    return S_OK;
}

HRESULT AddLinkInline(_In_ ABI::AdaptiveNamespace::IAdaptiveTextElement* adaptiveTextElement,
                      _In_ ABI::AdaptiveNamespace::IAdaptiveRenderContext* renderContext,
                      _In_ ABI::AdaptiveNamespace::IAdaptiveRenderArgs* renderArgs,
                      _In_ ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                      BOOL isBold,
                      BOOL isItalic,
                      _In_ IVector<ABI::Windows::UI::Xaml::Documents::Inline*>* inlines,
                      _Out_ UINT* characterLength)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap> attributeMap;
    RETURN_IF_FAILED(node->get_Attributes(&attributeMap));

    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> hrefNode;
    RETURN_IF_FAILED(attributeMap->GetNamedItem(HStringReference(L"href").Get(), &hrefNode));

    if (hrefNode == nullptr)
    {
        return E_INVALIDARG;
    }

    HString href;
    RETURN_IF_FAILED(GetTextFromXmlNode(hrefNode.Get(), href.GetAddressOf()));

    ComPtr<IUriRuntimeClassFactory> uriActivationFactory;
    RETURN_IF_FAILED(GetActivationFactory(HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(), &uriActivationFactory));

    ComPtr<IUriRuntimeClass> uri;
    RETURN_IF_FAILED(uriActivationFactory->CreateUri(href.Get(), uri.GetAddressOf()));

    ComPtr<ABI::Windows::UI::Xaml::Documents::IHyperlink> hyperlink =
        XamlHelpers::CreateXamlClass<ABI::Windows::UI::Xaml::Documents::IHyperlink>(
            HStringReference(RuntimeClass_Windows_UI_Xaml_Documents_Hyperlink));
    RETURN_IF_FAILED(hyperlink->put_NavigateUri(uri.Get()));

    ComPtr<ABI::Windows::UI::Xaml::Documents::ISpan> hyperlinkAsSpan;
    RETURN_IF_FAILED(hyperlink.As(&hyperlinkAsSpan));

    ComPtr<IVector<ABI::Windows::UI::Xaml::Documents::Inline*>> hyperlinkInlines;
    RETURN_IF_FAILED(hyperlinkAsSpan->get_Inlines(hyperlinkInlines.GetAddressOf()));

    RETURN_IF_FAILED(
        AddTextInlines(adaptiveTextElement, renderContext, renderArgs, node, isBold, isItalic, true, hyperlinkInlines.Get(), characterLength));

    ComPtr<ABI::Windows::UI::Xaml::Documents::IInline> hyperLinkAsInline;
    RETURN_IF_FAILED(hyperlink.As(&hyperLinkAsInline));
    RETURN_IF_FAILED(inlines->Append(hyperLinkAsInline.Get()));

    return S_OK;
}

HRESULT AddSingleTextInline(_In_ IAdaptiveTextElement* adaptiveTextElement,
                            _In_ IAdaptiveRenderContext* renderContext,
                            _In_ IAdaptiveRenderArgs* renderArgs,
                            _In_ HSTRING string,
                            bool isBold,
                            bool isItalic,
                            bool isInHyperlink,
                            _In_ IVector<ABI::Windows::UI::Xaml::Documents::Inline*>* inlines,
                            _Out_ UINT* characterLength)
{
    ComPtr<ABI::Windows::UI::Xaml::Documents::IRun> run = XamlHelpers::CreateXamlClass<ABI::Windows::UI::Xaml::Documents::IRun>(
        HStringReference(RuntimeClass_Windows_UI_Xaml_Documents_Run));
    RETURN_IF_FAILED(run->put_Text(string));

    ComPtr<ABI::Windows::UI::Xaml::Documents::ITextElement> runAsTextElement;
    RETURN_IF_FAILED(run.As(&runAsTextElement));

    RETURN_IF_FAILED(StyleTextElement(adaptiveTextElement, renderContext, renderArgs, isInHyperlink, runAsTextElement.Get()));

    if (isBold)
    {
        ComPtr<IAdaptiveHostConfig> hostConfig;
        RETURN_IF_FAILED(renderContext->get_HostConfig(&hostConfig));

        ABI::AdaptiveNamespace::FontStyle fontStyle;
        RETURN_IF_FAILED(adaptiveTextElement->get_FontStyle(&fontStyle));

        ABI::Windows::UI::Text::FontWeight boldFontWeight;
        RETURN_IF_FAILED(GetFontWeightFromStyle(hostConfig.Get(), fontStyle, ABI::AdaptiveNamespace::TextWeight::Bolder, &boldFontWeight));

        RETURN_IF_FAILED(runAsTextElement->put_FontWeight(boldFontWeight));
    }

    if (isItalic)
    {
        RETURN_IF_FAILED(runAsTextElement->put_FontStyle(ABI::Windows::UI::Text::FontStyle::FontStyle_Italic));
    }

    ComPtr<ABI::Windows::UI::Xaml::Documents::IInline> runAsInline;
    RETURN_IF_FAILED(run.As(&runAsInline));

    RETURN_IF_FAILED(inlines->Append(runAsInline.Get()));
    *characterLength = WindowsGetStringLen(string);

    return S_OK;
}

HRESULT AddTextInlines(_In_ IAdaptiveTextElement* adaptiveTextElement,
                       _In_ IAdaptiveRenderContext* renderContext,
                       _In_ IAdaptiveRenderArgs* renderArgs,
                       _In_ ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                       BOOL isBold,
                       BOOL isItalic,
                       bool isInHyperlink,
                       _In_ IVector<ABI::Windows::UI::Xaml::Documents::Inline*>* inlines,
                       _Out_ UINT* characterLength)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> childNode;
    RETURN_IF_FAILED(node->get_FirstChild(&childNode));

    UINT totalCharacterLength = 0;
    while (childNode != nullptr)
    {
        HString nodeName;
        RETURN_IF_FAILED(childNode->get_NodeName(nodeName.GetAddressOf()));

        INT32 isLinkResult;
        RETURN_IF_FAILED(WindowsCompareStringOrdinal(nodeName.Get(), HStringReference(L"a").Get(), &isLinkResult));

        INT32 isBoldResult;
        RETURN_IF_FAILED(WindowsCompareStringOrdinal(nodeName.Get(), HStringReference(L"strong").Get(), &isBoldResult));

        INT32 isItalicResult;
        RETURN_IF_FAILED(WindowsCompareStringOrdinal(nodeName.Get(), HStringReference(L"em").Get(), &isItalicResult));

        INT32 isTextResult;
        RETURN_IF_FAILED(WindowsCompareStringOrdinal(nodeName.Get(), HStringReference(L"#text").Get(), &isTextResult));

        UINT nodeCharacterLength;
        if (isLinkResult == 0)
        {
            RETURN_IF_FAILED(
                AddLinkInline(adaptiveTextElement, renderContext, renderArgs, childNode.Get(), isBold, isItalic, inlines, &nodeCharacterLength));
        }
        else if (isTextResult == 0)
        {
            HString text;
            RETURN_IF_FAILED(GetTextFromXmlNode(childNode.Get(), text.GetAddressOf()));
            RETURN_IF_FAILED(AddSingleTextInline(
                adaptiveTextElement, renderContext, renderArgs, text.Get(), isBold, isItalic, isInHyperlink, inlines, &nodeCharacterLength));
        }
        else
        {
            RETURN_IF_FAILED(AddTextInlines(adaptiveTextElement,
                                            renderContext,
                                            renderArgs,
                                            childNode.Get(),
                                            isBold || (isBoldResult == 0),
                                            isItalic || (isItalicResult == 0),
                                            isInHyperlink,
                                            inlines,
                                            &nodeCharacterLength));
        }

        ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> nextChildNode;
        RETURN_IF_FAILED(childNode->get_NextSibling(&nextChildNode));
        childNode = nextChildNode;
        totalCharacterLength += nodeCharacterLength;
    }

    *characterLength = totalCharacterLength;

    return S_OK;
}

HRESULT AddHtmlInlines(_In_ ABI::AdaptiveNamespace::IAdaptiveTextElement* adaptiveTextElement,
                       _In_ ABI::AdaptiveNamespace::IAdaptiveRenderContext* renderContext,
                       _In_ ABI::AdaptiveNamespace::IAdaptiveRenderArgs* renderArgs,
                       _In_ ABI::Windows::Data::Xml::Dom::IXmlNode* node,
                       bool isInHyperlink,
                       _In_ IVector<ABI::Windows::UI::Xaml::Documents::Inline*>* inlines,
                       _Out_ UINT* characterLength)
{
    ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> childNode;
    RETURN_IF_FAILED(node->get_FirstChild(&childNode));

    UINT totalCharacterLength = 0;
    while (childNode != nullptr)
    {
        HString nodeName;
        RETURN_IF_FAILED(childNode->get_NodeName(nodeName.GetAddressOf()));

        INT32 isOrderedListResult;
        RETURN_IF_FAILED(WindowsCompareStringOrdinal(nodeName.Get(), HStringReference(L"ol").Get(), &isOrderedListResult));

        INT32 isUnorderedListResult;
        RETURN_IF_FAILED(WindowsCompareStringOrdinal(nodeName.Get(), HStringReference(L"ul").Get(), &isUnorderedListResult));

        INT32 isParagraphResult;
        RETURN_IF_FAILED(WindowsCompareStringOrdinal(nodeName.Get(), HStringReference(L"p").Get(), &isParagraphResult));

        UINT nodeCharacterLength = 0;
        if ((isOrderedListResult == 0) || (isUnorderedListResult == 0))
        {
            RETURN_IF_FAILED(AddListInlines(
                adaptiveTextElement, renderContext, renderArgs, childNode.Get(), (isOrderedListResult == 0), isInHyperlink, inlines, &nodeCharacterLength));
        }
        else if (isParagraphResult == 0)
        {
            RETURN_IF_FAILED(AddTextInlines(
                adaptiveTextElement, renderContext, renderArgs, childNode.Get(), false, false, isInHyperlink, inlines, &nodeCharacterLength));
        }
        else
        {
            RETURN_IF_FAILED(
                AddHtmlInlines(adaptiveTextElement, renderContext, renderArgs, childNode.Get(), isInHyperlink, inlines, &nodeCharacterLength));
        }

        ComPtr<ABI::Windows::Data::Xml::Dom::IXmlNode> nextChildNode;
        RETURN_IF_FAILED(childNode->get_NextSibling(&nextChildNode));
        childNode = nextChildNode;
        totalCharacterLength += nodeCharacterLength;
    }

    *characterLength = totalCharacterLength;
    return S_OK;
}
