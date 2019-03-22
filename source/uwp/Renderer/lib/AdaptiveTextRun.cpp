#include "pch.h"
#include "AdaptiveTextRun.h"
#include "Util.h"
#include <windows.foundation.collections.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::AdaptiveNamespace;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::UI::Xaml;
using namespace ABI::Windows::UI::Xaml::Controls;

namespace AdaptiveNamespace
{
    HRESULT AdaptiveTextRun::RuntimeClassInitialize() noexcept try
    {
        RuntimeClassInitialize(std::make_shared<TextRun>());
        return S_OK;
    }
    CATCH_RETURN;

    HRESULT AdaptiveTextRun::RuntimeClassInitialize(const std::shared_ptr<AdaptiveSharedNamespace::TextRun>& sharedTextRun) noexcept try
    {
        m_highlight = sharedTextRun->GetHighlight();

        RETURN_IF_FAILED(GenerateActionProjection(sharedTextRun->GetSelectAction(), &m_selectAction));
        RETURN_IF_FAILED(AdaptiveTextElement::InitializeTextElement(sharedTextRun));
        return S_OK;
    }
    CATCH_RETURN;

    HRESULT AdaptiveTextRun::get_Highlight(boolean* highlight)
    {
        *highlight = m_highlight;
        return S_OK;
    }

    HRESULT AdaptiveTextRun::put_Highlight(boolean highlight)
    {
        m_highlight = highlight;
        return S_OK;
    }

    HRESULT AdaptiveTextRun::get_SelectAction(_COM_Outptr_ IAdaptiveActionElement** action)
    {
        return m_selectAction.CopyTo(action);
    }

    HRESULT AdaptiveTextRun::put_SelectAction(_In_ IAdaptiveActionElement* action)
    {
        m_selectAction = action;
        return S_OK;
    }

    HRESULT AdaptiveTextRun::GetSharedModel(std::shared_ptr<AdaptiveSharedNamespace::TextRun>& sharedModel) noexcept try
    {
        std::shared_ptr<AdaptiveSharedNamespace::TextRun> textRun = std::make_shared<AdaptiveSharedNamespace::TextRun>();
        RETURN_IF_FAILED(AdaptiveTextElement::SetTextElementProperties(textRun));

        textRun->SetHighlight(m_highlight);

        if (m_selectAction != nullptr)
        {
            std::shared_ptr<BaseActionElement> sharedAction;
            RETURN_IF_FAILED(GenerateSharedAction(m_selectAction.Get(), sharedAction));
            textRun->SetSelectAction(sharedAction);
        }

        sharedModel = textRun;
        return S_OK;
    }
    CATCH_RETURN;
}
