///@file
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>


#undef private

#define SET_NAME(stk)  \
{                      \
stk.set_name (#stk);   \
}                      \

#define TEST_MODE

typedef int elem_t;
#define data_dump printf("%d\n", data[i]);
static const int canary1_value    = 0xF023A4BA;
static const int canary2_value    = 0xF563F454;
static const elem_t canary3_value = ~0xAB;
static const elem_t canary4_value = ~0xC7;
static const int early_size       = 5;
static const elem_t $POISON       = ~0xAA;
static const int Stack_increase   = 10;
static const int Stack_decrease   = 13;  //decarease value must be more than increase one

#ifdef TEST_MODE
	typedef int elem_t;
#endif


#ifdef TEST_MODE
	#define GIVE_ERROR(CONDITION,ACTION)  \
		if (CONDITION)               \
		{							 \
			error = ACTION;			 \
			return ACTION;			 \
			*struct_sum = calc_struct_sum();		 \
		}
#else
	#define GIVE_ERROR(CONDITION,ACTION)  \
		if ( CONDITION ) return ACTION;
#endif

enum ERROR_CODE
{
	OK,
	DATA_SUM_IS_NOT_OK,
	SUM_STRUCT_IS_NOT_OK,
	STRUCT_CANARIES_FAULT,
	DATA_CANARIES_FAULT,
	STACK_OVERFLOW,
	STACK_UNDERFLOW,
	ALLOCATING_ERROR
};
//0 - OK
//1 - DATA SUM IS NOT OK
//2 - SUM_STRUCT IS NOT OK
//2 - STRUCT CANARIES ARE NOT OK
//3 - DATA CANARIES ARE NOT OK
//4 - STACK OVERFLOW
//5 - STACK UNDERFLOW
//6 - MEMORY CAN'T BE ALLOCATED

struct Stack_t
{
private:
	int canary1 = 0;         //initialized DEFEND
	long data_sum = 0;       //initialized DEFEND
	long size = 0;           //initialized
	long max_size = 3;       //initialized
	elem_t* data = nullptr;  //initialized
    const char* name = "YOU DIDN'T NAMED ME!!! use SET_NAME( object name )";
	long* struct_sum = 0;	 //initialized DEFEND
	int canary2 = 0;         //initialized DEFEND

	///@return Not commutative sum of data + 13*size - max_size
	long calc_data_sum ();

	///@return Sum of struct bytes
	long calc_struct_sum ();

	///@brief Function allocates new memory or delete unneccesary if it is needed
	///@return pointer to memory, 0 if it is impossible to allocate it
	elem_t* stackmem_check_allocation ();

	///@brief verificates our stack
	///@return error_code
	ERROR_CODE verification ();

	///@brief prints state of stack if something is not OK
	void diagnostic (ERROR_CODE error_code);

public:

	///@brief prints stack
	void print_stack();

	///@brief use SET_NAME( stack name )
	void set_name (char* name);

	///@brief push element to stack
	///@return true if element was pushed or false if element wasn't pushed
	bool push (elem_t new_elem);

	///@brief pops element off the stack
	///@return popped element_value
	elem_t pop ();

	///@return size_of_stack
	long tell_size ();

	Stack_t ();

	~Stack_t ();

    ERROR_CODE error = OK;
};



long Stack_t::calc_data_sum ()
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

long Stack_t::calc_struct_sum ()
	{
		char* adress = (char*) this;
		long sum = 0;
		char temp = 0;
		for (long i = 0; i < sizeof(Stack_t); i++)
        {
            temp = *(adress+i);
            sum += temp & 0x0F + temp >> 4;
        }
		return sum;
	}

elem_t* Stack_t::stackmem_check_allocation ()
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

ERROR_CODE Stack_t::verification ()
	{
		long temp_data_sum = calc_data_sum();
		long temp_struct_sum = calc_struct_sum ();
		GIVE_ERROR(temp_data_sum != data_sum, DATA_SUM_IS_NOT_OK);
		GIVE_ERROR(temp_struct_sum != *struct_sum, SUM_STRUCT_IS_NOT_OK);
		GIVE_ERROR(canary1 != canary1_value || canary2 != canary2_value, STRUCT_CANARIES_FAULT);
		GIVE_ERROR(data[0] != canary3_value || data[max_size+1] != canary4_value, DATA_CANARIES_FAULT);
		GIVE_ERROR(size > max_size + 1, STACK_OVERFLOW);
		GIVE_ERROR(size < 1, STACK_UNDERFLOW);
		elem_t* new_data = stackmem_check_allocation();
		if (new_data) data = new_data;
		else return ALLOCATING_ERROR;
		return OK;
	}

void Stack_t::diagnostic (ERROR_CODE error_code)
	{
		printf ("Stack name: %s\n", name);
		printf ("----------------------\nStack state: ");
		switch (error_code)
		{
			case (OK): printf ("OK\n"); break;

			case (DATA_SUM_IS_NOT_OK):    printf ("DATA SUM IS NOT OK, someone has attacked you :(\n");
			                              printf ("DATA SUM: %ld\n", data_sum);
			                              printf ("Data sum must be equal: %ld\n", calc_data_sum ());
			                              break;

			case (STRUCT_CANARIES_FAULT): printf ("STRUCT CANARIES ARE NOT OK, someone has attacked you :(\n");
			                              printf ("Actual first canary value: %d\n", canary1);
			                              printf ("Value that first canary must be equal: %d\n", canary1_value);
			                              printf ("Actual second canary value: %d\n", canary2);
			                              printf ("Value that second canary must be equal: %d\n", canary2_value);
			                              break;

			case (DATA_CANARIES_FAULT):   printf ("DATA CANARIES ARE NOT OK, someone has attacked you :(\n");
			                              printf ("Actual first canary value: %d\n", data[0]);
			                              printf ("Value that first canary must be equal: %d\n", canary3_value);
			                              printf ("Actual second canary value: %d\n", data[max_size+1]);
			                              printf ("Value that second canary must be equal: %d\n", canary4_value);
			                              break;

			case (STACK_OVERFLOW):        printf ("STACK OVERFLOWED\n");
                                          break;

			case (STACK_UNDERFLOW):       printf ("STACKED UNDERFLOWED\n");
                                          break;

			case (SUM_STRUCT_IS_NOT_OK):  printf ("STRUCT SUM IS NOT OK, someone has attacked you :(\n");
			                              printf ("STRUCT SUM: %ld\n", struct_sum);
                                          printf ("Struct sum must be equal: %ld\n", calc_struct_sum ());
			                              break;

			case (ALLOCATING_ERROR):      printf ("MEMORY FOR NEW DATA CAN'T BE ALLOCATED\n"); break;

			default:                      printf ("Wrong code:%d\n", error);
                                          break;
		}
		if (error_code != ALLOCATING_ERROR && error_code != SUM_STRUCT_IS_NOT_OK) print_stack();
		printf ("----------------------\n");
	}

#ifdef TEST_MODE
	ERROR_CODE error;
#endif

void Stack_t::print_stack()
	{
		printf ("STACK '%s':\n"
				"ADRESS OF STACK: %p\n", name, this);

		for (long i = 1; i <= size - 1; i++)
			if (data[i] != $POISON)
			{
				printf ("data [%.2ld] = ", i);
				data_dump
			}
			else
			{
				printf ("***data [%.2ld] = ", i);
				data_dump
			}

		printf ("EMPTY PART:\n");

		for (long i = size; i <= max_size; i++)
		{
				printf ("data [%.2ld] = ", i);
				data_dump
				-+printf (" That's empty data\n");
		}
		printf ("END OF STACK\n");
	}

void Stack_t::set_name (char* name)
	{
		this->name = name;
		*struct_sum = calc_struct_sum();
	}

bool Stack_t::push (elem_t new_elem)
	{
		ERROR_CODE error_code = verification ();
		if (error_code == OK)
		{
            data[size++] = new_elem;
            data_sum = calc_data_sum ();
            *struct_sum = calc_struct_sum ();
            return 1;
		}
		else
		{
            diagnostic (error_code);
            return 0;
	    }
	}
elem_t Stack_t::pop ()
	{
		ERROR_CODE error_code = verification();
		if (error_code == OK)
		{
            if ( size == 1 )
            {
                diagnostic (STACK_UNDERFLOW);
                return $POISON;
            }
			size--;
			elem_t pop_elem = data[size];
			data[size] = $POISON;
			data_sum = calc_data_sum ();
            *struct_sum = calc_struct_sum ();
			return pop_elem;
		}
		else
		{
			diagnostic (error_code);
			return $POISON;
		}
	}
long Stack_t::tell_size ()
	{
		return size - 1;
	}
Stack_t::Stack_t ()
	{
		canary1 = canary1_value;
		canary2 = canary2_value;
		data_sum = 13 - early_size;
		size = 1;
		max_size = early_size;
		data = (elem_t*) calloc (max_size+2, sizeof (elem_t));
		for (int i = 1; i <= max_size; i++) data[i] = $POISON;
		data[0] = canary3_value;
		data[max_size+1] = canary4_value;
		struct_sum = new long;
        *struct_sum = calc_struct_sum ();
	}
Stack_t::~Stack_t ()
	{
		if (data) free (data);
		data = nullptr;
		delete struct_sum;
		struct_sum = nullptr;
	}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef TEST_MODE
bool push_test1 ()
{
    char test_tmp = 0;
	Stack_t test_stk = {};
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
    char test_tmp = 0;
	Stack_t test_stk = {};
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
    Stack_t test_stk = {};
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
	Stack_t test_stk = {};
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
	Stack_t test_stk = {};
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
    printf ("\n!!!!!!SECOND ATTACK TEST WAS FAILED!!!!!!\n\n");
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
