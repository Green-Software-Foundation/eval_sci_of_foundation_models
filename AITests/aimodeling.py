# This program uses pipleline to load and run different foundation models from https://huggingface.co/. 
# Several gpt models released by EleutherAI are selected for our future experiments. To learn more about these models, please visit https://huggingface.co/EleutherAI 

import gc
from transformers import pipeline
from numba import jit, cuda
import time
import torch

gc.collect()
torch.cuda.empty_cache()
cuda.select_device(0)
cuda.close()
cuda.select_device(0)


with torch.no_grad():
	#genneo27 = pipeline('text-generation', model='EleutherAI/gpt-neo-2.7B', device=1)
	#gen13b = pipeline('text-generation', model='EleutherAI/gpt-neo-1.3B', device=1)
	#genj6b = pipeline('text-generation', model='EleutherAI/gpt-j-6B', device=-1)
	gen125m = pipeline('text-generation', model='EleutherAI/gpt-neo-125M', device=1)
	#gengpt2 = pipeline('text-generation', model='gpt2-large', device=1)

	#generators = [(gen13b,"gen13b"), (gen125m,"gen125m"), (gengpt2,"gengpt2"), (genneo27,"genneo27"), (genj6b,"genj6b") ]
	#generators = [(gen13b,"gen13b"), (gen125m,"gen125m"), (gengpt2,"gengpt2") ]
	generators = [(gen125m,"gen125m") ]
	text = ["What is an elephant?", "What is a lemur?", "How are you doing?", "Guess my weight", "Can you dance?", "Good day to you, sir", "What is a computer?", "Who is Elon Musk?", "Are you alive?", "Mad Max is a"]

	for generator in generators:
		filename = generator[1]
		print(generator[1],file=open(filename,"a"))
		gen = generator[0]
		count = 0
		while count < 1:
			for prompt in text:
				start_time = time.time()
				#result = generator(prompt, max_length=20, do_sample=True, temperature=0.4)
				result = gen(prompt, max_length=100, temperature=0.9)
				print(result, file=open(filename,"a"))
				print("--- %s seconds ---" % (time.time() - start_time), file=open(filename,"a"))
			count+=1
