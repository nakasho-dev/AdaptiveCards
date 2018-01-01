﻿//using AdaptiveCards;
//using AdaptiveCards.Rendering;
//using LiveCardAPI;
//using System;
//using System.Collections.Generic;
//using System.Linq;
//using System.Text;
//using System.Threading.Tasks;

//namespace LiveCardClient
//{
//    public class ServerEventBinder : AdaptiveVisitor
//    {
//        private ILiveCardServerAPI server;

//        public ServerEventBinder(ILiveCardServerAPI server)
//        {
//            this.server = server;
//        }

//        public static void Bind(ILiveCardServerAPI server, AdaptiveTypedElement el)
//        {
//            if (el is AdaptiveCard)
//                Bind(server, (AdaptiveCard)el);
//            else
//                Bind(server, (AdaptiveElement)el);
//        }

//        public static void Bind(ILiveCardServerAPI server, AdaptiveCard card)
//        {
//            new ServerEventBinder(server).VisitCard(card);
//        }

//        public static void Bind(ILiveCardServerAPI server, AdaptiveElement element)
//        {
//            new ServerEventBinder(server).Visit(null, element);
//        }

//        public override void Visit(AdaptiveTypedElement parent, AdaptiveElement adaptiveElement)
//        {
//            bind(adaptiveElement);
//            base.Visit(parent, adaptiveElement);
//        }

//        private void bind(AdaptiveElement element)
//        {
//            if (element.Events != null)
//            {
//                foreach (var eventName in element.Events)
//                {
//                    switch (eventName)
//                    {
//                        case "onClick":
//                            element.OnClick += Element_OnClick;
//                            break;
//                        case "onMouseEnter":
//                            element.OnMouseEnter += Element_OnMouseEnter;
//                            break;
//                        case "onMouseLeave":
//                            element.OnMouseLeave += Element_OnMouseLeave;
//                            break;
//                        default:
//                            if (element is AdaptiveInput)
//                            {
//                                AdaptiveInput input = element as AdaptiveInput;
//                                switch (eventName)
//                                {
//                                    case "onKey":
//                                        input.OnKey += Input_OnKey;
//                                        break;
//                                    case "onTextchanged":
//                                        input.OnTextChanged += Input_OnTextChanged;
//                                        break;
//                                    case "onSelectionChanged":
//                                        input.OnSelectionChanged += Input_OnSelectionChanged;
//                                        break;
//                                    case "onFocus":
//                                        input.OnFocus += Input_OnFocus;
//                                        break;
//                                    case "onBlur":
//                                        input.OnBlur += Input_OnBlur;
//                                        break;
//                                    default:
//                                        throw new ArgumentException($"{eventName} is not known");
//                                }
//                            }
//                            else
//                            {
//                                throw new ArgumentException($"{eventName} is not known");
//                            }
//                            break;
//                    }
//                }
//            }
//        }

//        private async void Input_OnBlur(object sender, EventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireBlur(element.Id);
//        }

//        private async void Input_OnFocus(object sender, EventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireFocus(element.Id);
//        }

//        private async void Input_OnSelectionChanged(object sender, SelectionChangedEventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireSelectionChanged(element.Id, e.Selection);
//        }

//        private async void Input_OnTextChanged(object sender, TextChangedEventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireTextChanged(element.Id, e.Text);
//        }

//        private async void Input_OnKey(object sender, KeyEventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireKey(element.Id, e.Key);
//        }

//        private async void Element_OnMouseLeave(object sender, EventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireMouseLeave(element.Id);
//        }

//        private async void Element_OnMouseEnter(object sender, EventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireMouseEnter(element.Id);
//        }

//        private async void Element_OnClick(object sender, EventArgs e)
//        {
//            AdaptiveElement element = (AdaptiveElement)sender;
//            await this.server.FireClick(element.Id);
//        }

//    }
//}