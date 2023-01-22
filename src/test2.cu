extern "C" __device__ void begin_parallel_region();
extern "C" __device__ void end_parallel_region();

extern "C" __device__ void kernel(int n, int* inputs, int* outputs)
{
	outputs[0] = 0;

	begin_parallel_region();
	for (int i = 1; i < n - 1; i++)
		if (inputs[i] < 2)
			outputs[i] = inputs[i] + i;
		else
			outputs[i] = inputs[i];
	end_parallel_region();

	outputs[n - 1] = n - 1;
}

int main() { return 0; }

