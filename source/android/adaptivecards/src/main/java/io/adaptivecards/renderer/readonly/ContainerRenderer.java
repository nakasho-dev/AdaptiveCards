package io.adaptivecards.renderer.readonly;

import android.content.Context;
import android.graphics.Color;
import android.nfc.Tag;
import android.support.v4.app.FragmentManager;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import io.adaptivecards.objectmodel.BackgroundImage;
import io.adaptivecards.objectmodel.ContainerStyle;
import io.adaptivecards.objectmodel.HeightType;
import io.adaptivecards.objectmodel.VerticalContentAlignment;
import io.adaptivecards.renderer.BackgroundImageLoaderAsync;
import io.adaptivecards.renderer.RenderArgs;
import io.adaptivecards.renderer.RenderedAdaptiveCard;
import io.adaptivecards.renderer.TagContent;
import io.adaptivecards.renderer.Util;
import io.adaptivecards.renderer.action.ActionElementRenderer;
import io.adaptivecards.renderer.actionhandler.ICardActionHandler;
import io.adaptivecards.objectmodel.BaseCardElement;
import io.adaptivecards.objectmodel.Container;
import io.adaptivecards.objectmodel.HostConfig;
import io.adaptivecards.renderer.BaseCardElementRenderer;
import io.adaptivecards.renderer.registration.CardRendererRegistration;

public class ContainerRenderer extends BaseCardElementRenderer
{
    protected ContainerRenderer()
    {
    }

    public static ContainerRenderer getInstance()
    {
        if (s_instance == null)
        {
            s_instance = new ContainerRenderer();
        }

        return s_instance;
    }

    @Override
    public View render(
            RenderedAdaptiveCard renderedCard,
            Context context,
            FragmentManager fragmentManager,
            ViewGroup viewGroup,
            BaseCardElement baseCardElement,
            ICardActionHandler cardActionHandler,
            HostConfig hostConfig,
            RenderArgs renderArgs)
    {
        Container container = null;
        if (baseCardElement instanceof Container)
        {
            container = (Container) baseCardElement;
        }
        else if ((container = Container.dynamic_cast(baseCardElement)) == null)
        {
            throw new InternalError("Unable to convert BaseCardElement to Container object model.");
        }

        ContainerStyle containerStyle = renderArgs.getContainerStyle();
        setSpacingAndSeparator(context, viewGroup, container.GetSpacing(),container.GetSeparator(), hostConfig, true /* horizontal line */);
        ContainerStyle styleForThis = container.GetStyle().swigValue() == ContainerStyle.None.swigValue() ? containerStyle : container.GetStyle();
        LinearLayout containerView = new LinearLayout(context);
        containerView.setTag(new TagContent(container));

        // Add this two for allowing children to bleed
        containerView.setClipChildren(false);
        containerView.setClipToPadding(false);

        if(!baseCardElement.GetIsVisible())
        {
            containerView.setVisibility(View.GONE);
        }

        containerView.setOrientation(LinearLayout.VERTICAL);
        if (container.GetHeight() == HeightType.Stretch)
        {
            containerView.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT, 1));
        }
        else
        {
            containerView.setLayoutParams(new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        }

        VerticalContentAlignment contentAlignment = container.GetVerticalContentAlignment();
        switch (contentAlignment)
        {
            case Center:
                containerView.setGravity(Gravity.CENTER_VERTICAL);
                break;
            case Bottom:
                containerView.setGravity(Gravity.BOTTOM);
                break;
            case Top:
            default:
                containerView.setGravity(Gravity.TOP);
                break;
        }

        if (!container.GetItems().isEmpty())
        {
            View v = CardRendererRegistration.getInstance().render(renderedCard,
                                                          context,
                                                          fragmentManager,
                                                          containerView,
                                                          container,
                                                          container.GetItems(),
                                                          cardActionHandler,
                                                          hostConfig,
                                                          renderArgs);

            if (v == null)
            {
                return null;
            }
        }

        if (styleForThis != containerStyle)
        {
            int padding = Util.dpToPixels(context, hostConfig.GetSpacing().getPaddingSpacing());
            containerView.setPadding(padding, padding, padding, padding);
            String color = hostConfig.GetBackgroundColor(styleForThis);
            containerView.setBackgroundColor(Color.parseColor(color));
        }

        BackgroundImage backgroundImageProperties = container.GetBackgroundImage();
        if (backgroundImageProperties != null && !backgroundImageProperties.GetUrl().isEmpty())
        {
            BackgroundImageLoaderAsync loaderAsync = new BackgroundImageLoaderAsync(
                    renderedCard,
                    context,
                    containerView,
                    hostConfig.GetImageBaseUrl(),
                    context.getResources().getDisplayMetrics().widthPixels,
                    backgroundImageProperties);

            loaderAsync.execute(backgroundImageProperties.GetUrl());
        }

        if (container.GetBleed() && (container.GetCanBleed() || (styleForThis != containerStyle || container.GetBackgroundImage() != null)))
        {
            long padding = Util.dpToPixels(context, hostConfig.GetSpacing().getPaddingSpacing());
            LinearLayout.LayoutParams layoutParams = (LinearLayout.LayoutParams) containerView.getLayoutParams();
            layoutParams.setMargins((int)-padding, layoutParams.topMargin, (int)-padding, layoutParams.bottomMargin);
            containerView.setLayoutParams(layoutParams);
        }

        if (container.GetSelectAction() != null)
        {
            containerView.setClickable(true);
            containerView.setOnClickListener(new ActionElementRenderer.ButtonOnClickListener(renderedCard, container.GetSelectAction(), cardActionHandler));
        }

        if (container.GetMinHeight() != 0)
        {
            containerView.setMinimumHeight(Util.dpToPixels(context, (int)container.GetMinHeight()));
        }

        viewGroup.addView(containerView);
        return containerView;
    }

    private static ContainerRenderer s_instance = null;
}
