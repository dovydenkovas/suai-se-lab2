INSERT INTO teacher (full_name, user_login, user_password)
VALUES ('Иванов Иван Иванович', 'example_login1', 'papAq5PwY/QQM');

INSERT INTO study_group (group_number)
VALUES ('z3431');

INSERT INTO student (group_id, full_name, user_login, user_password)
VALUES (
	(SELECT group_id FROM study_group WHERE group_number = 'z3431'),
	'Иванов Александр Петрович',
	'example_login2',
	'papAq5PwY/QQM'
);
