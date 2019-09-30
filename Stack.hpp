#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#define SET_NAME(stk)  \
{                      \
stk.set_name (#stk);   \
}                      \

//#define TEST_MODE

typedef int elem_t;
const int canary1_value    = 0xF023A4BA;
const int canary2_value    = 0xF563F454;
const elem_t canary3_value = ~0xAB;
const elem_t canary4_value = ~0xC7;
const int early_size       = 5;
const elem_t $POISON       = ~0xAA;
const int Stack_increase   = 10;
const int Stack_decrease   = 13;  //decarease value must be more than increase one

#ifdef TEST_MODE
	typedef int elem_t;
#endif

enum ERROR_CODE
{
	OK,
	SUM_IS_NOT_OK,
	STRUCT_CANARIES_FAULT,
	DATA_CANARIES_FAULT,
	STACK_OVERFLOW,
	STACK_UNDERFLOW,
	ALLOCATING_ERROR
};
//0 - OK
//1 - SUM IS NOT OK
//2 - STRUCT CANARIES ARE NOT OK
//3 - DATA CANARIES ARE NOT OK
//4 - STACK OVERFLOW
//5 - STACK UNDERFLOW
//6 - MEMORY CAN'T BE ALLOCATED

struct Stack_t
{
private:
	int canary1 = 0;         //initialized DEFEND
	long sum1 = 0;           //initialized DEFEND
	long size = 0;           //initialized
	long max_size = 3;       //initialized
	elem_t* data = nullptr;  //initialized
    char* name = "";
	long sum2 = 0;			 //initialized DEFEND
	int canary2 = 0;         //initialized DEFEND
	long calculate_sum ()
	{
		long sum = 0;
		for (long i = 1; i < size; i++)
		{
			if (i%2 == 1)
				sum+=data[i];
			else
				sum-=data[i];

		}
		sum += 13*size - max_size;
		return sum;
	}

	elem_t* stackmem_check_allocation ()
	{
		elem_t* new_data = NULL;
		if (size > max_size)
		{
			max_size += Stack_increase;
			new_data = (elem_t*) realloc (data, (max_size + 2) * sizeof (elem_t));
			for (int i = max_size - Stack_increase + 1; i <= max_size; i++)
			{
				data[i] = $POISON;
			}
			data[max_size+1] = canary4_value;
			return new_data;
		}

		if (max_size - size > Stack_decrease)
		{
			max_size -= Stack_decrease;
			new_data = (elem_t*) realloc (data, (max_size + 2) * sizeof (elem_t ));
            data[max_size+1] = canary4_value;
			return new_data;
		}

		return data;
	}

	ERROR_CODE verification ()
	{
	#ifdef TEST_MODE
		long temp_sum = calculate_sum();
		if ( temp_sum!=sum1 || temp_sum!=sum2 )
		{
		error = SUM_IS_NOT_OK;
		return SUM_IS_NOT_OK;
		}
		if ( canary1!=canary1_value || canary2!=canary2_value )
		{
		error = STRUCT_CANARIES_FAULT;
		return STRUCT_CANARIES_FAULT;
		}
		if ( data[0]!=canary3_value || data[max_size+1]!=canary4_value )
		{
		error = DATA_CANARIES_FAULT;
		return DATA_CANARIES_FAULT;
		}
		if ( size > max_size + 1 )
		{
		error = STACK_UNDERFLOW;
		return STACK_OVERFLOW;
		}
		if ( size < 1 )
		{
		error = STACK_UNDERFLOW;
		return STACK_UNDERFLOW;
		}
		elem_t* new_data = stackmem_check_allocation();
		if (new_data) data = new_data;
		else
		{
		error = ALLOCATING_ERROR;
		return ALLOCATING_ERROR;
		}
		error = OK;
		return OK;
	#else
		long temp_sum = calculate_sum();
		if ( temp_sum!=sum1 || temp_sum!=sum2 ) return SUM_IS_NOT_OK;
		if ( canary1!=canary1_value || canary2!=canary2_value ) return STRUCT_CANARIES_FAULT;
		if ( data[0]!=canary3_value || data[max_size+1]!=canary4_value ) return DATA_CANARIES_FAULT;
		if ( size > max_size + 1 ) return STACK_OVERFLOW;
		if ( size < 1 ) return STACK_UNDERFLOW;
		elem_t* new_data = stackmem_check_allocation();
		if (new_data) data = new_data;
		else return ALLOCATING_ERROR;
		return OK;
	#endif
	}

	void diagnostic (ERROR_CODE error_code)
	{
		printf ("Stack name: %s\n", name);
		printf ("----------------------\nStack state: ");
		switch (error_code)
		{
			case (OK): printf ("OK\n"); break;
			case (SUM_IS_NOT_OK): printf ("SUM IS NOT OK, someone has attacked you :(\n");
			                      printf ("FIRST SUM: %ld\n", sum1);
			                      printf ("SECOND SUM: %ld\n", sum2);
			                      printf ("Sum must be equal: %ld\n", calculate_sum ());
			                      break;
			case (STRUCT_CANARIES_FAULT): printf ("STRUCT CANARIES ARE NOT OK, someone has attacked you :(\n");
			                              printf ("Actual first canary value: %d\n", canary1);
			                              printf ("Value that first canary must be equal: %d\n", canary1_value);
			                              printf ("Actual second canary value: %d\n", canary2);
			                              printf ("Value that second canary must be equal: %d\n", canary2_value);
			                              break;
			case (DATA_CANARIES_FAULT): printf ("DATA CANARIES ARE NOT OK, someone has attacked you :(\n");
			                            printf ("Actual first canary value: %d\n", data[0]);
			                            printf ("Value that first canary must be equal: %d\n", canary3_value);
			                            printf ("Actual second canary value: %d\n", data[max_size+1]);
			                            printf ("Value that second canary must be equal: %d\n", canary4_value);
			                            break;
			case (STACK_OVERFLOW): printf ("STACK OVERFLOWED\n"); break;
			case (STACK_UNDERFLOW): printf ("STACKED UNDERFLOWED\n"); break;
			case (ALLOCATING_ERROR): printf ("MEMORY FOR NEW DATA CAN'T BE ALLOCATED\n"); break;
			default: printf ("Wrong code\n"); break;
		}
		if (error_code != ALLOCATING_ERROR) print_stack();
		printf ("----------------------\n");
	}

public:

#ifdef TEST_MODE
	ERROR_CODE error;
#endif

	void print_stack()
	{
		printf ("STACK:\n");
		for (long i = 1; i <= size - 1; i++)
			if (data[i] != $POISON)
			{
				printf ("data[%ld] = ", i);
				std::cout<<data[i]<<std::endl;
			}
			else
			{
				printf ("***data[%ld] = ", i);
				std::cout<<data[i]<<"  IT MAY BE EMPTY"<<std::endl;
			}

		printf ("EMPTY PART:\n");

		for (long i = size; i <= max_size; i++)
		{
				printf ("data[%ld] = ", i);
				std::cout<<data[i]<<" That's empty data"<<std::endl;
		}
		printf ("END OF STACK\n");
	}
	void set_name (char* name)
	{
		this->name = name;
	}
	void push (elem_t new_elem)
	{
		ERROR_CODE error_code = verification ();
		if (error_code == OK)
		{
            data[size++] = new_elem;
            sum1 = calculate_sum ();
            sum2 = calculate_sum ();
		}
		else diagnostic (error_code);
	}
	elem_t pop ()
	{
		ERROR_CODE error_code = verification();
		if (error_code == OK)
		{
            if ( size == 1 )
            {
                diagnostic (STACK_UNDERFLOW);
                return 0;
            }
			size--;
			elem_t pop_elem = data[size];
			data[size] = $POISON;
			sum1 = calculate_sum ();
            sum2 = calculate_sum ();
			return pop_elem;
		}
		else
		{
			diagnostic (error_code);
			return 0;
		}
	}
	long tell_size ()
	{
		return size - 1;
	}

	Stack_t ()
	{
		canary1 = canary1_value;
		canary2 = canary2_value;
		sum1 = 13 - early_size;
		sum2 = 13 - early_size;
		size = 1;
		max_size = early_size;
		data = (elem_t*) calloc (max_size+2, sizeof (elem_t));
		for (int i = 1; i <= max_size; i++) data[i] = $POISON;
		data[0] = canary3_value;
		data[max_size+1] = canary4_value;
	}

	~Stack_t ()
	{
		if (data) free (data);
		data = NULL;
	}
};




#ifdef TEST_MODE

bool push_test1 ()
{
    char test_tmp;
	Stack_t test_stk;
	SET_NAME (test_stk);
	for (char i = 10; i <= 100; i += 10)
	{
		test_stk.push ((elem_t)i);
	}
	if (char size = test_stk.tell_size () != 10)
	{
		printf ("FIRST TEST FAILED. SIZE IS NOT CORRECT\n");
		return false;
	}
    for (char i = 100; i >= 10; i -= 10)
	{
		test_tmp = (char) test_stk.pop();
		if (test_tmp != i)
		{
            printf ("FIRST TEST FAILED. PUSH IS NOT CORRECT\n");
            return false;
		}
	}
	printf ("First push test successfully done\n");
    return true;
}

bool push_test2 ()
{
    char test_tmp;
	Stack_t test_stk;
	SET_NAME (test_stk);
	for (char i = 1; i <= 12; i ++)
	{
		test_stk.push ((elem_t)i);
	}
	if (char size = test_stk.tell_size () != 12)
	{
		printf ("SECOND TEST FAILED. SIZE IS NOT CORRECT\n");
		return false;
	}
    for (char i = 12; i >= 1; i --)
	{
		test_tmp = (char) test_stk.pop();
		if (test_tmp != i)
		{
            printf ("SECOND TEST FAILED. PUSH IS NOT CORRECT\n");
            return false;
		}
	}
	printf ("Second push test successfully done\n");
    return true;
}

bool push_tests ()
{
    bool nice_stack = true;
    bool correct = true;
    if ( ! (correct = push_test1 ()) )
    {
    printf ("\n!!!!!!FIRST PUSH TEST WAS FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    if ( ! (correct = push_test2 ()) )
    {
    printf ("\n!!!!!!SECOND PUSH TEST WAS FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    return nice_stack;
}

bool pop_test1 ()
{
    Stack_t test_stk;
   	SET_NAME (test_stk);
    test_stk.push (10);
    test_stk.pop();
    test_stk.pop();
    int size = 0;
    if ( test_stk.error == STACK_UNDERFLOW )
    {
        printf ("FIRST TEST FAILED. POP IS NOT CORRECT\n");
        return 0;
    }
    printf ("First pop test successfully done\n");
    return 1;
}

bool pop_test2 ()
{
    Stack_t test_stk;
    SET_NAME (test_stk);
    test_stk.push (10);
    test_stk.push (43);
    test_stk.pop();
    int size = 0;
    if ( (size = test_stk.tell_size() ) != 1 )
    {
        printf ("SECOND TEST FAILED. POP IS NOT CORRECT\n");
        return 0;
    }
    printf ("Second pop test successfully done\n");
    return 1;
}

bool pop_tests ()
{
    bool nice_stack = true;
    bool correct = true;
    if ( ! (correct = pop_test1 ()) )
    {
    printf ("\n!!!!!!FIRST POP TEST WAS FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    if ( ! (correct = pop_test2 ()) )
    {
    printf ("\n!!!!!!SECOND POP TEST WAS FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    return nice_stack;
}

bool attack_test1 ()
{
	bool nice_stack = true;
	Stack_t test_stk;
	SET_NAME (test_stk);
	void* temp_pointer = &test_stk;
	char* executioner = (char*)temp_pointer;
   	printf ("\nGONNA ATTACK, WAIT FOR DIAGNOSTIC\n");
   	*(executioner+3) = 0;
   	test_stk.push(10);
   	printf ("ATTACK IS OVER\n");
   	if (test_stk.error != OK)
   	{
   		printf ("Attack detected. First test is successfully done\n");
    }
   	else
   	{
   		printf ("Attack is not detected. First test is failed\n");
   		nice_stack = false;
   	}
   	return nice_stack;
}

bool attack_test2 ()
{
	bool nice_stack = true;
	Stack_t test_stk;
	SET_NAME (test_stk);
	void* temp_pointer = &test_stk;
	char* executioner = (char*)temp_pointer;
   	printf ("\nGONNA ATTACK, WAIT FOR DIAGNOSTIC\n");
   	for (int i = 2; i < 6; i++)
   		*(executioner+i) = 0;
   	test_stk.push(3);
   	printf ("ATTACK IS OVER\n");
   	if (test_stk.error != OK)
   	{
   		printf ("Attack detected. First test is successfully done\n");
    }
   	else
   	{
   		printf ("Attack is not detected. First test is failed\n");
   		nice_stack = false;
   	}
   	return nice_stack;
}


bool attack_tests ()
{
    bool nice_stack = true;
    bool correct = true;
    if ( ! (correct = attack_test1 ()) )
    {
    printf ("\n!!!!!!FIRST ATTACK TEST WAS FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    if ( ! (correct = attack_test2 ()) )
    {
    printf ("\n!!!!!!SECOND PUSH TEST WAS FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    return nice_stack;
}

bool stack_tests ()
{
    bool nice_stack = true;
    bool correct = true;
    if ( ! (correct = pop_tests ()) )
    {
    printf ("\n!!!!!!POP TESTS WERE FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    if ( ! (correct = push_tests ()) )
    {
    printf ("\n!!!!!!PUSH TESTS WERE FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    if ( ! (correct = attack_tests ()) )
    {
    printf ("\n!!!!!!ATTACK TESTS WERE FAILED!!!!!!\n\n");
    nice_stack = false;
    }
    if (nice_stack == true) printf ("\n\nYOUR STACK IS OK\n\n\n");
    else printf ("!!!!!!Your stack is not OK!!!!!!");
    return nice_stack;
}
#endif
