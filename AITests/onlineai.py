# Gradio provides a covienent way to demo machine learning models with a friendly web interface. 
# This python script shows how to demo foundation models via Gradio.

import gradio as gr
from transformers import pipeline

#generator = pipeline('text-generation', model='EleutherAI/gpt-neo-2.7B')
#generator = pipeline('text-generation', model='EleutherAI/gpt-neo-1.3B')
#gpt_j_6B = pipeline('text-generation', model='EleutherAI/gpt-j-6B')
#gpt2_large = pipeline('text-generation', model='EleutherAI/gpt-neo-125M')
#gpt2_large = pipeline('text-generation', model='gpt2-large')

title = "Natural Language Processor"
description = "NLP using various transformer models to capture energy usage."
article="Coming soon"

examples = [
  ["gpt2", "The Moon's orbit around the Earth has"],
  ["gpt2", 'The tower is 342 meters (1,063 ft) tall,'],
  ["gpt2", 'The smooth Borealis basin in the Northern Hemisphere covers 40%'],
  ["gpt-j-6B", "The Moon's orbit around the Earth has"],
  ["gpt-j-6B", 'The tower is 342 meters (1,063 ft) tall,'],
  ["gpt-j-6B", 'The smooth Borealis basin in the Northern Hemisphere covers 40%']
]

#gpt2 = gr.Interface.load("huggingface/gpt2-large")
#gptj6B = gr.Interface.load("huggingface/EleutherAI/gpt-j-6B")

#gpt2 = pipeline('text-generation', model='gpt2-large')
#gptj6B = pipeline('text-generation', model='EleutherAI/gpt-j-6B')
gptneo125M = pipeline('text-generation', model='EleutherAI/gpt-neo-125M')


def fn(model_choice, input):
  if model_choice=="gpt2":
    return gpt2(input)
  elif model_choice=="gpt-j-6B":
    return gptj6B(input)
  elif model_choice=="gpt-neo-125M":
    return gptneo125M(input)

gr.Interface(fn, [gr.inputs.Dropdown(["gpt2", "gpt-j-6B", "gpt-neo-125M"]), "text"], "text", examples=examples, title=title, description=description, article=article).launch(debug=True, share=True)
