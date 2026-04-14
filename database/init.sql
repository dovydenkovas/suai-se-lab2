-- Тип статуса отчета
CREATE TYPE report_status AS ENUM ('SENT', 'ACCEPTED', 'REJECTED');

-- Преподаватели
CREATE TABLE teacher (
	teacher_id SERIAL PRIMARY KEY,
	full_name VARCHAR(60) NOT NULL,
	user_login VARCHAR(60) NOT NULL UNIQUE,
	user_password VARCHAR(255) NOT NULL
);

-- Учебные группы
CREATE TABLE study_group (
	group_id SERIAL PRIMARY KEY,
	group_number VARCHAR(10) NOT NULL UNIQUE
);

-- Студенты
-- Группу нельзя удалить, пока есть студенты
CREATE TABLE student (
	student_id SERIAL PRIMARY KEY,
	group_id INT NOT NULL,
	full_name VARCHAR(60) NOT NULL,
	user_login VARCHAR(60) NOT NULL UNIQUE,
	user_password VARCHAR(255) NOT NULL,
	CONSTRAINT fk_student_group
		FOREIGN KEY (group_id)
		REFERENCES study_group(group_id)
		ON DELETE RESTRICT
);

-- Предметы
-- Нельзя удалить преподавателя, если есть предметы
CREATE TABLE subject (
	subject_id SERIAL PRIMARY KEY,
	teacher_id INT NOT NULL,
	name VARCHAR(100) NOT NULL,
	CONSTRAINT fk_subject_teacher
		FOREIGN KEY (teacher_id)
		REFERENCES teacher(teacher_id)
		ON DELETE RESTRICT
);

-- Задания
-- Группу нельзя удалить если, есть задания
-- Преподавателя и предмет нельзя удалить, если есть задания
CREATE TABLE task (
	task_id SERIAL PRIMARY KEY,
	group_id INT NOT NULL,
	teacher_id INT NOT NULL,
	subject_id INT NOT NULL,
	title varchar(180) NOT NULL,
	description TEXT NOT NULL DEFAULT '',
	CONSTRAINT fk_task_group
		FOREIGN KEY (group_id)
		REFERENCES study_group(group_id)
		ON DELETE RESTRICT,
	CONSTRAINT fk_task_teacher
		FOREIGN KEY (teacher_id)
		REFERENCES teacher(teacher_id)
		ON DELETE RESTRICT,
	CONSTRAINT fk_task_subject
		FOREIGN KEY (subject_id)
		REFERENCES subject(subject_id)
		ON DELETE RESTRICT
);

-- Отчеты
-- Удаляются при удалении студента или задания
CREATE TABLE report (
	report_id SERIAL PRIMARY KEY,
	task_id INT NOT NULL,
	student_id INT NOT NULL,
	text TEXT NOT NULL DEFAULT '',
	status report_status NOT NULL DEFAULT 'SENT',
	grade INT,
	CONSTRAINT chk_report_grade 
		CHECK (
			(status != 'ACCEPTED' AND grade IS NULL)
			OR
			(status != 'ACCEPTED' AND grade BETWEEN 1 AND 5)
		),
	CONSTRAINT fk_report_task
		FOREIGN KEY (task_id)
		REFERENCES task(task_id)
		ON DELETE CASCADE
	CONSTRAINT fk_report_student
		FOREIGN KEY (student_id)
		REFERENCES student(student_id)
		ON DELETE CASCADE
	CONSTRAINT uq_report_task_student
		UNIQUE (task_id, student_id)
);
