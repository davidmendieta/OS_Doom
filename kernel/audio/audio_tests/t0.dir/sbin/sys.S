	# int write(int, void *, unsigned int)
	.global write
write:
	mov $1,%eax
	int $48
	ret

	# int shutdown(void)
	.global shutdown
shutdown:
	mov $7,%eax
	int $48
	ret

	# void exit(int)
	.global exit
exit:
	mov $0,%eax
	int $48
	ret


