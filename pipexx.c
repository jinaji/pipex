/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pipexx.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jinkim2 <jinkim2@student.42seoul.kr>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/29 19:35:29 by jinkim2           #+#    #+#             */
/*   Updated: 2022/07/04 16:37:33 by jinkim2          ###   ########seoul.kr  */
/*                                                                            */
/* ************************************************************************** */

#include "pipexx.h"

void	ft_error(char *str)
{
	perror(str);
	exit (1);
}

void	free_tmp(char **tmp)
{
	int	i;

	i = 0;
	while (tmp[i])
	{
		free (tmp[i]);
		i++;
	}
	free (tmp);
}

int	dp_cnt(char *str)
{
	char	**tmp;
	int		i;

	i = 0;
	tmp = ft_split(str, ' ');
	while (tmp[i])
		i++;
	free_tmp(tmp);
	return (i);
}

void	split_cmd_op(t_argv *arg, char **tmp, int i, int j)
{
	while (tmp[j])
	{
		arg->cmd[i][j] = ft_strdup(tmp[j]);
		j++;
	}
	arg->cmd[i][j] = 0;
	printf("%s\n", arg->cmd[i][j]);
}

void	split_cmd(t_argv *arg, char **av, int ac)
{
	char	**tmp;
	int		h;
	int		i;
	int		j;

	h = 0;
	i = h;
	if (arg->h_flag)
		h = 1;
	while (ac - 3 > h)
	{
		j = 0;
		tmp = ft_split(av[h + 2], ' ');
		arg->cmd[i] = (char **)malloc(sizeof(char *) * (dp_cnt(av[h + 2]) + 1));
		if (!(arg->cmd[i]))
			ft_error("malloc error");
		split_cmd_op(arg, tmp, i, j);
		free_tmp(tmp);
		i++;
		h++;
	}
	arg->cmd[i] = 0;
}

void	get_path(t_argv *arg, char **envp)
{
	char	*tmp;
	int		i;

	i = 0;
	while (envp[i])
	{
		if (ft_strnstr(envp[i], "PATH=", 5))
			tmp = ft_strdup(envp[i] + 5);
		i++;
	}
	if (tmp)
		arg->path = ft_split(tmp, ':');
	free (tmp);
}

char	*join_path(char *path, char *cmd)
{
	char	*tmp;
	char	*tmp2;

	tmp = ft_strjoin(path, "/");
	tmp2= ft_strjoin(tmp, cmd);
	free (tmp);
	return (tmp2);
}

void	command_not_found(t_argv *arg)
{
	perror("command not found");
	arg->no_cmd = 0;
}

void	get_cmd_path(t_argv *arg)
{
	char	*tmp;
	int		i;
	int		j;

	j = 0;
	// if (arg->h_flag)
	// 	j = 1;
	printf ("cmd cnt %d\n", arg->cmd_cnt);
	while (j < arg->cmd_cnt)
	{
		i = 0;
		arg->no_cmd = 0;
		while (arg->path[i])
		{
			tmp = join_path(arg->path[i], arg->cmd[j][0]); // segfault with j
			if (access(tmp, F_OK) == 0)
			{
				arg->cmd_path[j] = ft_strdup(tmp);
				arg->no_cmd = 1;
			}
			i++;
			free (tmp);
		}
		if (!arg->no_cmd)
			command_not_found(arg);
		j++;
	}
}

int	ft_strcmp(char *str, char *str2)
{
	int	i;

	i = 0;
	while (str[i])
	{
		if (str[i] != str2[i])
			return (0);
		i++;
	}
	if (str2[i])
		return (0);
	return (1);
}

void	check_valid(t_argv *arg)
{
	if (arg->h_flag)
	{
		arg->inf_fd = open("tmp", O_RDWR | O_CREAT | O_TRUNC, 0644);
		arg->out_fd = open(arg->outfile, O_RDWR | O_CREAT | O_APPEND, 0644);
	}
	else
	{
		arg->inf_fd = open(arg->infile, O_RDONLY);
		arg->out_fd = open(arg->outfile, O_RDWR | O_CREAT | O_TRUNC, 0644);
	}
	if (arg->inf_fd == -1)
		ft_error ("infile open error");
	if (arg->out_fd == -1)
		ft_error("outfile open error");
	get_cmd_path(arg);
}

void	arg_init(t_argv *arg, int ac, char **av, char **envp)
{
	ft_memset(arg, 0, sizeof(t_argv));
	arg->cmd_cnt = ac - 3;
	arg->cmd = (char ***)malloc(sizeof(char **) * (ac - 2));
	if (!(arg->cmd))
		ft_error("malloc error");
	arg->cmd_path = (char **)malloc(sizeof(char *) * (ac - 2));
	arg->infile = ft_strdup(av[1]);
	arg->outfile = ft_strdup(av[ac - 1]);
	arg->envp = envp;
	if (ft_strcmp(arg->infile, "here_doc"))
	{
		arg->h_flag = 1;
		arg->limiter = ft_strjoin(av[2], "\n");
		arg->cmd_cnt -= 1;
	}
	split_cmd(arg, av, ac);
	get_path(arg, envp);
	check_valid(arg);
	// printf("%s\n", arg->cmd[0][0]);
	// printf("%s\n", arg->cmd_path[0]);
	// printf("%s\n", arg->cmd[1][0]);
	// printf("%s\n", arg->cmd_path[1]);
}

void	make_tmp_file(t_argv *arg)
{
	char	*tmp;

	tmp = get_next_line(0);
	while (!ft_strcmp(tmp, arg->limiter))
	{
		write (arg->inf_fd, tmp, ft_strlen(tmp));
		tmp = get_next_line(0);
	}
	close (arg->inf_fd);
	arg->inf_fd = open ("tmp", O_RDONLY);
	if (arg->inf_fd == -1)
		ft_error ("tmp open error");
	dup2(arg->inf_fd, STDIN_FILENO);
	close (arg->inf_fd);
}

void	first_cmd_exec(t_argv *arg, int fd[2])
{
	int	i;

	i = 0;
	// if (arg->h_flag)
	// 	i = 1;
	printf("first\n");
	
	// char	buff[1000];
	// for (int i = 0; buff[i]; i++)
	// 	buff[i] = 0;
	// read (arg->inf_fd, buff, 1000);
	// printf("%s\n", buff); 여기서 읽힘 이걸 가지고 cmd 1 실행해야함 !!
	
	printf("%s\n", arg->cmd[i][0]);
	printf("%s\n", arg->cmd_path[i]);
	// printf("%s\n", arg->cmd[i + 1][0]);
	// printf("%s\n", arg->cmd_path[i + 1]);

	close(fd[READ]);
	// printf("%d\n", arg->inf_fd);
	dup2(arg->inf_fd, STDIN_FILENO);
	dup2(fd[WRITE], STDOUT_FILENO);
	close(fd[WRITE]);
	execve(arg->cmd_path[i], arg->cmd[i], arg->envp);
}

void	middle_cmd_exec(t_argv *arg, int fd[2], int fd2[2], int i)
{
	printf("middle %d\n", i);
	int	h_i;
	printf("%s\n", arg->cmd_path[i]);
	for (int k = 0; arg->cmd[i][k]; k++)
		printf("%s\n", arg->cmd[i][k]);
	h_i = i;
	// if (arg->h_flag)
	// 	h_i += 1;
	printf ("%d\n", h_i);
	if (h_i % 2) // a read b write
	{
		close(fd[WRITE]);
		close(fd2[READ]);
		dup2(fd[READ], STDIN_FILENO);
		close(fd[READ]);
		dup2(fd2[WRITE], STDOUT_FILENO);
		close(fd2[WRITE]);
	}
	else
	{
		close(fd2[WRITE]);
		close(fd[READ]);
		dup2(fd2[READ], STDIN_FILENO);
		close(fd2[READ]);
		dup2(fd[WRITE], STDOUT_FILENO);
		close(fd[WRITE]);
	}
	execve(arg->cmd_path[i], arg->cmd[i], arg->envp);
}

void	last_cmd_exec(t_argv *arg, int fd[2], int fd2[2], int i)
{
	printf ("last %d \n", i);
	printf("path %s\n", arg->cmd_path[i]);
	for (int k = 0; arg->cmd[i][k]; k++)
		printf("%s\n", arg->cmd[i][k]);
	// printf("i %d cmd cnt %d\n", i, arg->cmd_cnt);
	// char	buff[1000];
	// for (int i = 0; buff[i]; i++)
	// 	buff[i] = 0;
	// read (fd[WRITE], buff, 1000);
	// printf("%s\n", buff);
	int	h_i;

	h_i = i;
	// printf ("c i h %d %d %d \n", arg->cmd_cnt, i, h_i);
	// if (arg->cmd[i + 1])
	// 	ft_error("command count error");
	// if (arg->h_flag)
	// 	h_i += 1;
	if (h_i % 2)
	{
		close(fd2[READ]);
		dup2(fd[READ], STDIN_FILENO);
		close(fd[READ]);
	}
	else
	{
		close(fd[READ]);
		dup2(fd2[READ], STDIN_FILENO);
		close(fd2[READ]);
	}
	dup2 (arg->out_fd, STDOUT_FILENO);
	execve(arg->cmd_path[i], arg->cmd[i], arg->envp);
}

void	excute_cmds(t_argv *arg, int fd[2], int fd2[2], int *i)
{
	pid_t	pid;
	int		status;

	if (*i % 2)
		close(fd[WRITE]);
	else
		close(fd2[WRITE]);
	pid = fork();
	if (pid == 0)
		middle_cmd_exec(arg, fd, fd2, *i);
	else
	{
		waitpid(pid, &status, 0);
		*i += 1;
	}
}

void	execute_cmd(t_argv *arg)
{
	pid_t	pid;	
	int		status;
	int		fd[2];
	int		fd2[2];
	int		i;

	i = 1;
	if (pipe(fd) == -1 || pipe(fd2) == -1)
		ft_error ("pipe error");
	if (arg->h_flag)
		make_tmp_file(arg);
	printf ("i : %d \n", i);
	pid = fork();
	if (pid == 0)
	{
		// if (arg->h_flag)
		// 	make_tmp_file(arg);
		first_cmd_exec(arg, fd);
	}
	else
	{
		waitpid(pid, &status, 0);
		while (arg->cmd_cnt - 1 > i)
			excute_cmds(arg, fd, fd2, &i);
		close (fd[WRITE]);
		close (fd2[WRITE]);
		last_cmd_exec(arg, fd, fd2, i);
	}
}

// void	h_execute_cmd(t_argv *arg)
// {
// 	pid_t	pid;
// 	char	*tmp;
// 	int		status;
// 	int		fd[2];

// 	pipe(fd);
// 	tmp = get_next_line(0);
// 	while (!(ft_strcmp(tmp, arg->limiter)))
// 	{
// 		write (fd[WRITE], tmp, ft_strlen(tmp));
// 		tmp = get_next_line(0);
// 	}
// 	dup2(arg->inf_fd, STDIN_FILENO);
// 	pid = fork();
// 	if (pid == 0)
// 	{
// 		close(fd[WRITE]);
// 		first_cmd_exec(arg, fd);
// 	}
// 	else
// 	{
// 		waitpid(pid, &status, 0);
// 	}
// }

int	main(int ac, char **av, char **envp)
{
	t_argv	arg;

	if (ac < 5)
		ft_error("wrong format");
	arg_init(&arg, ac, av, envp);
	if (arg.h_flag && ac < 6)
		ft_error("wrong format");
	// if (arg.h_flag)
	// 	h_execute_cmd(&arg);
	execute_cmd(&arg);
}
