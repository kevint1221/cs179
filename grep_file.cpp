JNIEXPORT jstring JNICALL
Java_com_example_cs179fdemo_MainActivity_grep(JNIEnv *env, jobject this, jstring grep_keywords) {
    // Get a file descriptor for `/dev/ion` to interface with ION.
    int fd = ion_open();

    if(fd < 0){
        return (*env)->NewStringUTF(env, "Failed to open `/dev/ion`");
    }

    __android_log_print(ANDROID_LOG_DEBUG, ION_TAG,
            "Successfully opened `/dev/ion`: fd == %d", fd);

    ion_user_handle_t handle;

    // Allocate memory from the DMA heap.
    int ret = ion_alloc(fd, getpagesize(), 0, (1 << ION_QSECOM_HEAP_ID),
            ION_HEAP_TYPE_DMA, &handle);

    if(ret < 0){
        ion_close(fd);
        return (*env)->NewStringUTF(env, "Failed to allocate from DMA heap");
    }

    __android_log_print(ANDROID_LOG_DEBUG, ION_TAG, "Successfully allocated from DMA heap");

    struct ion_fd_data fd_data = {
            .fd = -1,
            .handle = handle
    };

    // Create file descriptors to implement shared memory.
    ret = ion_share(fd, &fd_data);

    if(ret < 0){
        ion_close(fd);
        return (*env)->NewStringUTF(env, "Failed to create fds for shared memory");
    }

    __android_log_print(ANDROID_LOG_DEBUG, ION_TAG, "Successfully created fds for shared memory");

    void *start_addr = mmap((void *)0, getpagesize(), PROT_READ | PROT_WRITE,
            MAP_SHARED, fd_data.fd, 0);

    __android_log_print(ANDROID_LOG_DEBUG, ION_TAG, "Successfully mapped to user space");

    // No longer need the fd, so close it.
    ion_close(fd);

    // `mmap()` failed, so return an empty string.
    if(start_addr == MAP_FAILED){
        __android_log_print(ANDROID_LOG_DEBUG, ION_TAG, "mmap() returned -1 (error): %s\n",
                strerror(errno));

        return (*env)->NewStringUTF(env, "mmap() failed");
    }

    char* data = (char*) start_addr;

    __android_log_print(ANDROID_LOG_DEBUG, ION_TAG,
            "Start mmap loc.: %x (%d)\n", (unsigned int)data, (int)data);

    // Test if dummy string exists in the mapped memory.
    char test_str[] = "hello";
    int size = 5;
    int cur_index = 0;

    for(int i = 0; i < getpagesize(); ++i) {
        if((*(data + i)) == test_str[cur_index]) {
            __android_log_print(ANDROID_LOG_DEBUG, ION_TAG, "Found %c\n", test_str[cur_index]);

            ++cur_index;

            // Found continuous dummy string.
            if(cur_index == size){
                // start over/find more.
                cur_index = 0;
            }
        }
        else { // Current character did not match, so start over.
            cur_index = 0;
        }
    }

    ret = munmap(start_addr, getpagesize());

    // `munmap()` failed.
    if(ret < 0){
        __android_log_print(ANDROID_LOG_DEBUG, ION_TAG,
                "Failed to unmap mem. region: %s\n", strerror(errno));

        return (*env)->NewStringUTF(env, "Unmap failed");
    }

    __android_log_print(ANDROID_LOG_DEBUG, ION_TAG, "Successfully unmapped");

    // Return dummy string for now.
    return (*env)->NewStringUTF(env, "Success");
}
