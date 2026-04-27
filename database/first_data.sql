INSERT INTO teacher (full_name, user_login, user_password)
VALUES ('Иванов Иван Иванович', 'teacher', 'paN8aiEIonqJE');

INSERT INTO study_group (group_number)
VALUES ('z3431');

insert into subject (name, teacher_id, subject_id) values ('Физика', 2, 1);
insert into task (description, group_id, subject_id, teacher_id, title) values ('Отвечайте честно.', 1, 1, 2, 'Вы любите физику?');

INSERT INTO student (group_id, full_name, user_login, user_password)
VALUES (
	(SELECT group_id FROM study_group WHERE group_number = 'z3431'),
	'Иванов Александр Петрович',
	'student',
	'paN8aiEIonqJE'
);
