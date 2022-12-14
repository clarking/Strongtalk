//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#ifdef __linux__

#include <signal.h>

#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utility/LongInteger64.hpp"
#include "vm/platform/os.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "posix.hpp"

Lock::Lock(pthread_mutex_t *mutex) :
		_mutex(mutex) {
	pthread_mutex_lock(mutex);
}

Lock::~Lock() {
	pthread_mutex_unlock(_mutex);
}

void Event::signal() {
	Lock mark(&_mutex);
	_signalled = true;
	pthread_cond_signal(&_notifier);
}

void Event::reset() {
	Lock mark(&_mutex);
	_signalled = false;
	pthread_cond_signal(&_notifier);
}

bool Event::waitFor() {
	Lock mark(&_mutex);
	while (!_signalled)
		pthread_cond_wait(&_notifier, &_mutex);
	return true;
}

Event::Event(bool state) {
	_signalled = state;
	pthread_mutex_init(&_mutex, nullptr);
	pthread_cond_init(&_notifier, nullptr);
}

Event::~Event() {
	pthread_mutex_destroy(&_mutex);
	pthread_cond_destroy(&_notifier);
}

const auto STACK_SIZE = ThreadStackSize * 1024;

void os_dump_context2(ucontext_t *context) {
	mcontext_t mcontext = context->uc_mcontext;
	SPDLOG_INFO("EAX: %x", mcontext.gregs[REG_EAX]);
	SPDLOG_INFO("EBX: %x", mcontext.gregs[REG_EBX]);
	SPDLOG_INFO("ECX: %x", mcontext.gregs[REG_ECX]);
	SPDLOG_INFO("EDX: %x", mcontext.gregs[REG_EDX]);
	SPDLOG_INFO("EIP: %x", mcontext.gregs[REG_EIP]);
	SPDLOG_INFO("ESP: %x", mcontext.gregs[REG_ESP]);
	SPDLOG_INFO("EBP: %x", mcontext.gregs[REG_EBP]);
	SPDLOG_INFO("EDI: %x", mcontext.gregs[REG_EDI]);
	SPDLOG_INFO("ESI: %x", mcontext.gregs[REG_ESI]);
}

void os_dump_context() {
	ucontext_t context;
	getcontext(&context);
	os_dump_context2(&context);
}

static std::int32_t main_thread_id;

static std::int32_t _argc;
static char **_argv;

std::int32_t os::argc() {
	return _argc;
}

char **os::argv() {
	return _argv;
}

void os::set_args(std::int32_t argc, char *argv[]) {
	_argc = argc;
	_argv = argv;
}

GrowableArray<Thread *> *Thread::_threads = nullptr;
static Thread *main_thread;

extern void intercept_for_single_step();

std::int32_t os::getenv(const char *name, char *buffer, std::int32_t len) {
	return 0;
}

bool os::move_file(const char *from, const char *to) {
	return false;
}

bool os::check_directory(const char *dir_name) {
	return false;
}

void os::breakpoint() {
	asm("int3");
}

Thread *os::starting_thread(std::int32_t *id_addr) {
	*id_addr = main_thread->_thread_index;
	return main_thread;
}

typedef struct {

	std::int32_t (*main)(void *parameter);

	void *parameter;
	char *stackLimit;

} thread_args_t;

static Event *threadCreated = nullptr;

char *calcStackLimit() {
	char *stackptr;
	asm("movl %%esp, %0;" : "=a"(stackptr));
	stackptr = (char *) align(stackptr, os::vm_page_size());

	std::int32_t stackHeadroom = 2 * os::vm_page_size();
	return stackptr - STACK_SIZE + stackHeadroom;
}

void *mainWrapper(void *args) {
	thread_args_t *targs = (thread_args_t *) args;
	targs->stackLimit = calcStackLimit();

	std::int32_t (*threadMain)(void *) = targs->main;
	void *parameter = targs->parameter;
	std::int32_t *result = (std::int32_t *) malloc(sizeof(std::int32_t));
	threadCreated->signal();
	*result = threadMain(parameter);
	return (void *) result;
}

Thread *os::create_thread(std::int32_t threadStart(void *parameter), void *parameter, std::int32_t *id_addr) {

	pthread_t threadId;
	thread_args_t threadArgs;
	{
		ThreadCritical tc;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, STACK_SIZE);

		threadCreated->reset();
		threadArgs.main = threadStart;
		threadArgs.parameter = parameter;

		std::int32_t status = pthread_create(&threadId, &attr, &mainWrapper, &threadArgs);
		if (status != 0) {
			st_fatal1("Unable to create thread. status = %d", status);
		}
	}
	threadCreated->waitFor();
	Thread *thread = new Thread(threadId, threadArgs.stackLimit);
	*id_addr = thread->_thread_index;
	return thread;
}

void *os::stack_limit(Thread *thread) {
	return thread->_stackLimit;
}

void os::terminate_thread(Thread *thread) {
}

void os::delete_event(Event *event) {
	delete event;
}

Event *os::create_event(bool initial_state) {
	return new Event(initial_state);
}

tms processTimes;

std::int32_t os::updateTimes() {
	return times(&processTimes) != (clock_t) -1;
}

double os::userTime() {
	return ((double) processTimes.tms_utime) / CLOCKS_PER_SEC;
}

double os::systemTime() {
	return ((double) processTimes.tms_stime) / CLOCKS_PER_SEC;
}

double os::user_time_for(Thread *thread) {
	//Hack warning - assume half time is spent in kernel, half in user code
	return thread->get_cpu_time() / 2;
}

double os::system_time_for(Thread *thread) {
	//Hack warning - assume half time is spent in kernel, half in user code
	return thread->get_cpu_time() / 2;
}

static std::int32_t has_performance_count = 0;
static LongInteger64 initial_performance_count(0, 0);
static LongInteger64 performance_frequency(0, 0);

void os::fatalExit(std::int32_t num) {
	exit(num);
}

dll_func_ptr_t os::dll_lookup(const char *name, DLL *library) {
	return library->lookup(name);
}

DLL *os::dll_load(const char *name) {
	DLL *library = new DLL(name);
	if (library->isValid())
		return library;
	delete library;
	return nullptr;
}

bool os::dll_unload(DLL *library) {
	delete library;
	return true;
}

const char *os::dll_extension() {
	return ".so";
}

std::int32_t _nCmdShow = 0;

void *os::get_hInstance() {
	return (void *) nullptr;
}

void *os::get_prevInstance() {
	return (void *) nullptr;
}

std::int32_t os::get_nCmdShow() {
	return 0;
}

extern std::int32_t bootstrappingInProgress;

void os::timerStart() {
}

void os::timerStop() {
}

void os::timerPrintBuffer() {
}

char *os::reserve_memory(std::int32_t size) {
	return (char *) mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

bool os::commit_memory(const char *addr, std::int32_t size) {
	return !mprotect(const_cast<char *>( addr ), size, PROT_READ | PROT_WRITE);
}

bool os::uncommit_memory(const char *addr, std::int32_t size) {
	return !mprotect(const_cast<char *>(addr), size, PROT_NONE);
}

bool os::release_memory(const char *addr, std::int32_t size) {
	return !munmap((char *) addr, size);
}

bool os::guard_memory(const char *addr, std::int32_t size) {
	return false;
}

void *os::malloc(std::int32_t size) {
	return ::malloc(size);
}

const char *os::exec_memory(std::int32_t size) {
	return (const char *) mmap(0, size, PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
}

void *os::calloc(std::int32_t size, char filler) {
	return ::calloc(size, filler);
}

void os::free(void *p) {
	::free(p);
}

void os::transfer(Thread *from_thread, Event *from_event, Thread *to_thread, Event *to_event) {
	from_event->reset();
	to_event->signal();
	from_event->waitFor();
}

void os::transfer_and_continue(Thread *from_thread, Event *from_event, Thread *to_thread, Event *to_event) {
	from_event->reset();
	to_event->signal();
}

void os::suspend_thread(Thread *thread) {
	os_dump_context();
	pthread_kill(thread->_threadId, SIGUSR1);
}

void suspendHandler(std::int32_t signum) {
	Thread *current = Thread::find(pthread_self());
	st_assert(current, "Suspended thread not found");
	current->suspend();
}

void os::resume_thread(Thread *thread) {
	thread->resume();
}

void os::sleep(std::int32_t ms) {
}

void os::fetch_top_frame(Thread *thread, std::int32_t **sp, std::int32_t **fp, char **pc) {
}

std::int32_t os::current_thread_id() {
	Thread *currentThread = Thread::find(pthread_self());
	if (currentThread == nullptr)
		return -1;

	return currentThread->_thread_index;
}

void os::wait_for_event(Event *event) {
	event->waitFor();
}

void os::reset_event(Event *event) {
	event->reset();
}

void os::signal_event(Event *event) {
	event->signal();
}

bool os::wait_for_event_or_timer(Event *event, std::int32_t timeout_in_ms) {
	return false;
}

extern "C" bool WizardMode;

void process_settings_file(const char *file_name, bool quiet);

static std::int32_t number_of_ctrl_c = 0;

std::int32_t os::_vm_page_size = getpagesize();

LongInteger64 os::elapsed_counter() {
	struct timespec current_time;
	clock_gettime(CLOCK_REALTIME, &current_time);
	int64_t current64 = ((int64_t) current_time.tv_sec) * 1e9 + current_time.tv_nsec;
	uint high = current64 >> 32;
	uint low = current64 & 0xffffffff;
	LongInteger64 current(low, high);
	return current;
}

LongInteger64 os::elapsed_frequency() {
	return LongInteger64(1e9, 0);
}

static struct timespec initial_time;

double os::elapsedTime() {

	struct timespec current_time;
	clock_gettime(CLOCK_REALTIME, &current_time);
	std::int32_t seconds = current_time.tv_sec - initial_time.tv_sec;
	std::int32_t nanoseconds = current_time.tv_nsec - initial_time.tv_nsec;

	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 1e9;
	}

	return seconds + (nanoseconds / 1e9);
}

static void initialize_performance_counter() {
	clock_gettime(CLOCK_REALTIME, &initial_time);
}

void os::initialize_system_info() {
	Thread::init();
	main_thread = new Thread(pthread_self(), calcStackLimit());
	initialize_performance_counter();
}

std::int32_t os::message_box(const char *title, const char *message) {
	return 0;
}

const char *os::platform_class_name() {
	return "UnixPlatform";
}

std::int32_t os::error_code() {
	return errno;
}

extern "C" bool EnableTasks;

pthread_mutex_t ThreadSection;

bool ThreadCritical::_initialized = false;

void ThreadCritical::intialize() {
	pthread_mutex_init(&ThreadSection, nullptr);
	_initialized = true;
}

void ThreadCritical::release() {
	pthread_mutex_destroy(&ThreadSection);
}

ThreadCritical::ThreadCritical() {
	pthread_mutex_lock(&ThreadSection);
}

ThreadCritical::~ThreadCritical() {
	pthread_mutex_unlock(&ThreadSection);
}

void real_time_tick(std::int32_t delay_time);

void *watcherMain(void *ignored) {

	const struct timespec delay = {0, 1 * 1000 * 1000};
	constexpr std::int32_t delay_interval = 1; // Delay 1 ms

	while (1) {
		std::int32_t status = nanosleep(&delay, nullptr);
		if (!status)
			return 0;
		real_time_tick(delay_interval);
	}
	return 0;
}

void segv_repeated(std::int32_t signum, siginfo_t *info, void *context) {
	SPDLOG_INFO("SEGV during signal handling. Aborting.");
	exit(EXIT_FAILURE);
}

void install_dummy_handler() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	sa.sa_sigaction = segv_repeated;
	if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
		SPDLOG_INFO("SIGSEGV\n");
	}
}

void trace_stack(std::int32_t thread_id);

void (*userHandler)(void *fp, void *sp, void *pc) = nullptr;

static void handler(std::int32_t signum, siginfo_t *info, void *context) {

	install_dummy_handler();
	trace_stack(os::current_thread_id());

	if (!userHandler) {
		SPDLOG_INFO("signal: {:d}\ninfo: %x\ncontext: %x", signum, (std::int32_t) info, (std::int32_t) context);
		os_dump_context2((ucontext_t *) context);
	}
	else {
		mcontext_t mcontext = ((ucontext_t *) context)->uc_mcontext;
		userHandler((void *) mcontext.gregs[REG_EBP], (void *) mcontext.gregs[REG_ESP], (void *) mcontext.gregs[REG_EIP]);
	}

	exit(EXIT_FAILURE);
}

void os::add_exception_handler(void newHandler(void *fp, void *sp, void *pc)) {
	userHandler = newHandler;
}

void install_signal_handlers() {
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; // Restart functions if interrupted by handler
	sa.sa_handler = suspendHandler;
	if (sigaction(SIGUSR1, &sa, nullptr) == -1) {
		SPDLOG_INFO("SIGUSR1\n");
	}

	sa.sa_flags |= SA_SIGINFO;
	sa.sa_sigaction = handler;
	if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
		SPDLOG_INFO("SIGSEGV\n");
	}

}

void os_init() {

	SPDLOG_INFO("system-init:  os_init");

	ThreadCritical::intialize();

	install_signal_handlers();
	os::initialize_system_info();

	pthread_setconcurrency(1);

	threadCreated = new Event(false);

	if (EnableTasks) {
		pthread_t watcherThread;
		std::int32_t status = pthread_create(&watcherThread, nullptr, &watcherMain, nullptr);
		if (status != 0) {
			st_fatal("Unable to create thread");
		}
	}
}

void os_exit() {
	ThreadCritical::release();
}

#endif
