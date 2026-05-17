// ==========================
//  CONFIG  — Change CGI_BASE to your actual endpoint
// ==========================
const CGI_BASE = '/cgi-bin/api.cgi';
// const API_PATH = 'http://localhost:8787/proxy';
const API_PATH = 'http://atuin.space:8888';
// App state 
const state = {
    user: null,
    token: null,
    currentTask: null,
    currentReport: null,
};
document.addEventListener('DOMContentLoaded', () => {
    const savedState = JSON.parse(localStorage.getItem('state') || '{}');
    if (savedState.token) {
        state.user = savedState.user;
        state.token = savedState.token;
        applySession();
    } else {
        showPage('pageHome');
    }
});
// ==========================
//  UTILITIES
// ==========================
function showPage(id) {
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.getElementById(id).classList.add('active');
}

function goHome() {
    if (!state.user) showPage('pageHome');
    else if (state.user.role === 'student') loadStudentTasks();
    else if (state.user.role === 'teacher') loadTeacherTasks();
}

function setMsg(elId, text, type) {
    const el = document.getElementById(elId);
    el.textContent = text;
    el.className = 'msg show ' + (type === 'ok' ? 'msg-success' : 'msg-error');
}

function statusLabel(s) {
    return { SENT: 'Отправлен', ACCEPTED: 'Принят', REJECTED: 'Отклонён' }[s] || s;
}

function badgeHtml(status) {
    return `<span class="badge badge-${status}">${statusLabel(status)}</span>`;
}



// ====== DATA FOR OFFLINE MODE ======
const offlineData = {
    teacherTasks: [
        { task_id: 1, title: 'Лабораторная 1', subject_name: 'Основы программирования', group_number: 'Б01-001', description: 'Сделать задание 1.' },
        { task_id: 2, title: 'Лабораторная 2', subject_name: 'Основы программирования', group_number: 'Б01-001', description: 'Сделать задание 2.' }
    ],
    studentTasks: [
        { task_id: 1, title: 'Лабораторная 1', subject_name: 'Основы программирования', teacher_name: 'Иван Иванов', report_status: 'SENT', report_grade: null, group_number: 'Б01-001', report_text: 'Мой ответ на ЛР1' },
        { task_id: 2, title: 'Лабораторная 2', subject_name: 'Основы программирования', teacher_name: 'Иван Иванов', report_status: 'REJECTED', report_grade: null, group_number: 'Б01-001', report_text: '' }
    ]
};
let offlineTaskId = 3;

let isOfflineMode = false;

async function apiGet(path) {
    if (isOfflineMode) {
        // OFFLINE MODE
        if (path === '/teacher/tasks') {
            return JSON.parse(JSON.stringify(offlineData.teacherTasks));
        }
        if (path.startsWith('/teacher/tasks/')) {
            const id = parseInt(path.split('/').pop());
            return JSON.parse(JSON.stringify(offlineData.teacherTasks.find(t => t.task_id === id)));
        }
        if (path === '/tasks') {
            return JSON.parse(JSON.stringify(offlineData.studentTasks));
        }
        if (path.startsWith('/tasks/')) {
            const id = parseInt(path.split('/').pop());
            const t = offlineData.studentTasks.find(t => t.task_id === id);
            if (!t) throw new Error('not found');
            return Object.assign({}, t, { description: offlineData.teacherTasks.find(tt => tt.task_id === id)?.description });
        }
        if (path.startsWith('/reports')) {
            return [];
        }
        throw new Error('OFFLINE: Not implemented for ' + path);
    } else {
        // REAL API
        const r = await fetch(API_PATH + CGI_BASE + path, {
            headers: {
                'Authorization': `Bearer ${state.token}`
            }
        });
        if (!r.ok) throw new Error(`HTTP ${r.status}`);
        return r.json();
    }
}

async function apiPost(path, data) {
    console.log(data)
    if (isOfflineMode) {
        // OFFLINE MODE
        if (path === '/teacher/tasks') {
            // create
            const newTask = {
                task_id: offlineTaskId++,
                title: data.title,
                description: data.description,
                group_number: data.group_number,
                subject_name: data.subject_name
            };
            offlineData.teacherTasks.push(newTask);
            offlineData.studentTasks.push({
                task_id: newTask.task_id,
                title: newTask.title,
                subject_name: newTask.subject_name,
                teacher_name: 'Иван Иванов',
                report_status: null,
                report_grade: null,
                group_number: newTask.group_number,
                report_text: ''
            });
            return { ok: true };
        }
        if (path.startsWith('/teacher/tasks/') && !path.endsWith('/delete')) {
            // edit
            const id = parseInt(path.split('/').pop());
            const t = offlineData.teacherTasks.find(t => t.task_id === id);
            if (t) {
                t.title = data.title;
                t.description = data.description;
                t.group_number = data.group_number;
                t.subject_name = data.subject_name;
                // sync for student
                const st = offlineData.studentTasks.find(st => st.task_id === id);
                if (st) {
                    st.title = data.title;
                    st.subject_name = data.subject_name;
                    st.group_number = data.group_number;
                }
            }
            return { ok: true };
        }
        if (path.endsWith('/delete')) {
            // delete
            const id = parseInt(path.split('/').slice(-2)[0]);
            offlineData.teacherTasks = offlineData.teacherTasks.filter(t => t.task_id !== id);
            offlineData.studentTasks = offlineData.studentTasks.filter(t => t.task_id !== id);
            return { ok: true };
        }
        throw new Error('OFFLINE: Not implemented for ' + path);
    } else {
        // REAL API
        console.log(state.token);

        const r = await fetch(API_PATH + CGI_BASE + path, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${state.token}`
            },
            body: JSON.stringify(data),
        });
        console.log('Response status:', r.status);
        let res = await r.json();
        res.status = r.status;
        if (!r.ok) throw new Error(`HTTP ${r.status}`);
        console.log({
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${state.token}`
            },
            body: JSON.stringify(data),
        });
        return res;
    }
}

// ==========================
//  AUTH
// ==========================
async function doLogin() {
    const login = document.getElementById('loginInput').value.trim();
    const password = document.getElementById('passwordInput').value;

    if (login == 1 && password == 1 && isOfflineMode) {
        state.user = { id: 1, full_name: 'Иван Иванов', role: 'teacher' };
        state.token = 'offline-token-for-teacher';
        localStorage.setItem('state', JSON.stringify(state));
    }
    else if (login == 2 && password == 2 && isOfflineMode) {
        state.user = { id: 2, full_name: 'Пётр Петров', role: 'student', group_number: 'Б01-001' };
        state.token = 'offline-token-for-student';
        localStorage.setItem('state', JSON.stringify(state));
    }
    else {
        try {
            const data = await apiPost('/auth/login', { login, password });
            state.user = data.user;
            state.token = data.token;
            console.log("respond:");
            console.log(data.status);
            console.log(data.statusLabel);
            localStorage.setItem('state', JSON.stringify(state));
        } catch (e) {
            setMsg('authError', 'Неверный логин или пароль.', 'err');
            return;
        }
    }

    applySession();
}

function applySession() {
    document.getElementById('authBar').style.display = 'none';
    const ub = document.getElementById('userBar');
    ub.style.display = 'flex';
    document.getElementById('userChip').textContent =
        state.user.full_name + (state.user.role === 'student' ? ' (студент)' : state.user.role === 'teacher' ? ' (преподаватель)' : 'неизвестная роль)');

    if (state.user.role === 'student') loadStudentTasks();
    else if (state.user.role === 'teacher') loadTeacherTasks();
}
// ==========================
//  TEACHER — TASKS PAGE
// ==========================
async function loadTeacherTasks() {
    showPage('pageTeacherTasks');
    const wrap = document.getElementById('teacherTaskTableWrap');
    wrap.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка заданий…</div>';
    try {
        // GET /teacher/tasks
        // Expected: [ { task_id, title, subject_name, group_number, description } ]
        const tasks = await apiGet('/teacher/tasks');
        renderTeacherTaskTable(tasks, wrap);
    } catch (e) {
        wrap.innerHTML = `<div class="empty-state"><div class="es-icon">⚠</div><p>Не удалось загрузить задания.</p></div>`;
    }
}

function renderTeacherTaskTable(tasks, wrap) {
    if (!tasks.length) {
        wrap.innerHTML = `<div class="empty-state"><div class="es-icon">📋</div><p>Заданий пока нет.</p></div>`;
        return;
    }
    wrap.innerHTML = `
        <table>
            <thead><tr>
                <th>#</th>
                <th>Название</th>
                <th>Группа</th>
                <th>Предмет</th>
                // <th>Действия</th>
            </tr></thead>
            <tbody>
                ${tasks.map(t => `
                    <tr>
                        <td>${t.task_id}</td>
                        <td><a href="#" onclick="showTaskAnswers(${t.task_id});return false;">${escHtml(t.title)}</a></td>
                        <td>${escHtml(t.group_number || '—')}</td>
                        <td>${escHtml(t.subject_name || '—')}</td>
                        // <td>
                            // <button class="btn-primary" onclick="editTask(${t.task_id})">Редактировать</button>
                        // </td>
                    </tr>`).join('')}
            </tbody>
        </table>`;
}

// Students answers page
async function showTaskAnswers(taskId) {
    showPage('pageTaskDetail');
    const card = document.getElementById('taskDetailCard');
    card.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка…</div>';
    let task = {};
    let answers = [];
    if (isOfflineMode) {
        task = (await apiGet('/teacher/tasks/' + taskId)) || {};
        answers = offlineData.studentTasks.filter(s => s.task_id === taskId && s.report_status);
    } else {
        task = await apiGet('/teacher/tasks/' + taskId);
        answers = await apiGet(`/reports/${taskId}`);
    }
    showTaskAnswers._lastTaskId = taskId;

    card.innerHTML = `
        <h3>${escHtml(task.title || 'Задание #' + taskId)}</h3>
        <div class="detail-meta">
            <span>📚 ${escHtml(task.subject_name || '—')}</span>
            <span>🎓 Группа: ${escHtml(task.group_number || '—')}</span>
        </div>
        <div class="detail-body">${escHtml(task.description || '(описание отсутствует)')}</div>
        <div class="divider"></div>
        <h4>Ответы студентов</h4>
        ${answers.length === 0 ? '<div class="empty-state"><p>Ответов пока нет.</p></div>' :
            answers.map((a, idx) => `
            <div class="answer-block" style="margin-bottom:1.5rem; line-height: 1.8;">
                <div style="font-size:0.97rem; margin-bottom: 0.75rem;"><b>${escHtml(a.student?.full_name || a.student_name || 'Студент')}</b> ${a.status ? badgeHtml(a.status) : ''}</div>
                <div style="margin:0.75rem 0; color:var(--ink-soft); white-space:pre-wrap; line-height: 1.8;">${escHtml(a.text || a.report_text || '(нет текста)')}</div>
                <div style="margin-top:1rem; line-height: 1.8;">
                <label for="statusSelect_${idx}" style="font-size:0.88rem; color:var(--ink-faint); margin-right:0.5rem; display: block; margin-bottom: 0.5rem;">Статус:</label>
                <select id="statusSelect_${idx}" data-answer-idx="${idx}"
                    onchange="document.getElementById('gradeWrapper_${idx}').style.display = this.value === 'REJECTED' || this.value === '' ? 'none' : 'inline-flex';">
                    <option value="" >Выбрать статус</option>
                    <option value="ACCEPTED" ${a.status === 'ACCEPTED' ? 'selected' : ''}>Принят</option>
                    <option value="REJECTED" ${a.status === 'REJECTED' ? 'selected' : ''}>Не принят</option>
                </select>
                <span id="gradeWrapper_${idx}" style="display: ${a.status === 'REJECTED' || a.status === 'SENT' ? 'none' : 'inline-flex'}; align-items:center; margin-left:1rem; margin-top: 0.75rem;">
                    <label for="gradeInput_${idx}" style="font-size:0.88rem; color:var(--ink-faint); margin-right:0.5rem; margin-top: 0.75rem;">Оценка:</label>
                    <input type="number" id="gradeInput_${idx}" value="${a.grade ?? ''}" min="0" max="100" style="width:4rem;">
                </span>
                </div>
                <button class="btn-primary" onclick="onApplyAnswerChange(${taskId}, ${idx})" style="margin-top:1rem;">Изменить</button>
            </div>
            `).join('')}
    `;
    document.getElementById('reportFormWrap').style.display = 'none';
}

window.onApplyAnswerChange = async function (taskId, idx) {
    const statusSelect = document.getElementById(`statusSelect_${idx}`);
    const gradeInput = document.getElementById(`gradeInput_${idx}`);
    const newStatus = statusSelect.value;
    const newGrade = gradeInput.value;

    if (!newStatus) {
        alert('Выберите статус перед изменением.');
        return;
    }

    if (isOfflineMode) {
        let answers = offlineData.studentTasks.filter(s => s.task_id === taskId && s.report_status);
        if (answers[idx]) {
            answers[idx].report_status = newStatus;
            answers[idx].report_grade = newGrade;
        }
        showTaskAnswers(taskId);
    } else {
        // send POST /api/reports/{taskId} {status, grade}
        const answers = await apiGet(`/reports/${taskId}`);
        const answer = answers[idx];
        if (answer && answer.report_id) {
            const resp = await apiPost(`/reports/${answer.report_id}`, { status: newStatus, grade: newGrade });
            console.log('Update report response:', resp);
            showTaskAnswers(taskId);
        }
        else {
            alert('Ошибка: не найден отчёт для этого ответа.');
        }
    }
}

window.onChangeAnswerStatus = async function (taskId, idx, newStatus) {
    const gradeInput = document.getElementById(`gradeInput_${idx}`);
    const grade = gradeInput ? gradeInput.value : null;
    if (isOfflineMode) {
        const answers = offlineData.studentTasks.filter(s => s.task_id === taskId && s.report_status);
        if (answers[idx]) {
            answers[idx].report_status = newStatus;
            answers[idx].report_grade = grade;
        }
        showTaskAnswers(taskId);
    } else {
        // send POST /api/reports/{report_id} {status, grade}
        const answers = await apiGet(`/reports/${taskId}`);
        const answer = answers[idx];
        if (answer && answer.report_id) {
            await apiPost(`/reports/${answer.report_id}`, { status: newStatus, grade: grade });
            showTaskAnswers(taskId);
        }
    }
}

window.onChangeGrade = async function (taskId, idx, newGrade) {
    const statusSelect = document.getElementById(`statusSelect_${idx}`);
    const status = statusSelect ? statusSelect.value : null;
    if (isOfflineMode) {
        const answers = offlineData.studentTasks.filter(s => s.task_id === taskId && s.report_status);
        if (answers[idx]) {
            answers[idx].report_grade = newGrade;
        }
        showTaskAnswers(taskId);
    } else {
        // send POST /api/reports/{report_id} {status, grade}
        const answers = await apiGet(`/reports/${taskId}`);
        const answer = answers[idx];
        if (answer && answer.report_id) {
            await apiPost(`/reports/${answer.report_id}`, { status: status, grade: newGrade });
            showTaskAnswers(taskId);
        }
    }
}

window.onChangeAnswerStatus = async function (taskId, idx, newStatus) {
    if (isOfflineMode) {
        const answers = offlineData.studentTasks.filter(s => s.task_id === taskId && s.report_status);
        if (answers[idx]) {
            answers[idx].report_status = newStatus;
        }
        showTaskAnswers(taskId);
    } else {
        // send POST /api/reports/{report_id} {status}
        const answers = await apiGet(`/reports/${taskId}`);
        const answer = answers[idx];
        if (answer && answer.report_id) {
            await apiPost(`/reports/${answer.report_id}`, { status: newStatus });
            showTaskAnswers(taskId);
        }
    }
}


let editingTaskId = null;

async function openTaskEditor() {
    editingTaskId = null;
    document.getElementById('taskEditorTitle').textContent = 'Новое задание';
    document.getElementById('taskTitleInput').value = '';
    document.getElementById('taskDescInput').value = '';
    document.getElementById('taskGroupInput').value = '';
    document.getElementById('taskSubjectInput').value = '';
    // document.getElementById('btnDeleteTask').style.display = 'none';
    document.getElementById('taskEditorMsg').className = 'msg';
    document.getElementById('taskEditorMsg').textContent = '';
    showPage('pageTaskEditor');
}

async function editTask(taskId) {
    editingTaskId = taskId;
    showPage('pageTaskEditor');
    document.getElementById('taskEditorTitle').textContent = 'Редактировать задание';
    // document.getElementById('btnDeleteTask').style.display = 'inline-block';
    document.getElementById('taskEditorMsg').className = 'msg';
    document.getElementById('taskEditorMsg').textContent = '';
    try {
        // GET /teacher/tasks/:id
        const t = await apiGet(`/teacher/tasks/${taskId}`);
        document.getElementById('taskTitleInput').value = t.title || '';
        document.getElementById('taskDescInput').value = t.description || '';
        document.getElementById('taskGroupInput').value = t.group_number || '';
        document.getElementById('taskSubjectInput').value = t.subject_name || '';
    } catch (e) {
        setMsg('taskEditorMsg', 'Ошибка загрузки задания.', 'err');
    }
}

async function saveTask() {
    const title = document.getElementById('taskTitleInput').value.trim();
    const description = document.getElementById('taskDescInput').value.trim();
    const group_number = document.getElementById('taskGroupInput').value.trim();
    const subject_name = document.getElementById('taskSubjectInput').value.trim();
    if (!title || !group_number || !subject_name) {
        setMsg('taskEditorMsg', 'Заполните все обязательные поля.', 'err');
        return;
    }
    document.getElementById('btnSaveTask').disabled = true;
    document.getElementById('taskEditorMsg').className = 'msg';
    try {
        if (editingTaskId) {
            // POST /teacher/tasks/:id (edit)
            const resp = await apiPost(`/teacher/tasks/${editingTaskId}`, { title, description, group_number, subject_name });
            if (resp.status === 201) {
                setMsg('taskEditorMsg', 'Задание обновлено!', 'ok');
            }
            else {
                setMsg('taskEditorMsg', 'Ошибка при добавлении задания. Проверьте данные.', 'err');
            }
        } else {
            // POST /teacher/tasks (create)
            const resp = await apiPost('/teacher/tasks', { title, description, group_number, subject_name });
            if (resp.status === 201) {
                setMsg('taskEditorMsg', 'Задание добавлено!', 'ok');
            }
            else {
                setMsg('taskEditorMsg', 'Ошибка при добавлении задания. Проверьте данные.', 'err');
            }
        }
        setTimeout(loadTeacherTasks, 700);
    } catch (e) {
        setMsg('taskEditorMsg', 'Ошибка при сохранении.', 'err');
    } finally {
        document.getElementById('btnSaveTask').disabled = false;
    }
}

async function deleteTask() {
    if (!editingTaskId) return;
    if (!confirm('Удалить это задание?')) return;
    document.getElementById('btnDeleteTask').disabled = true;
    try {
        // POST /teacher/tasks/delete/:id
        await apiPost(`/teacher/tasks/delete/${editingTaskId}`, {});
        setMsg('taskEditorMsg', 'Задание удалено!', 'ok');
        setTimeout(loadTeacherTasks, 700);
    } catch (e) {
        setMsg('taskEditorMsg', 'Ошибка при удалении.', 'err');
    } finally {
        document.getElementById('btnDeleteTask').disabled = false;
    }
}

function doLogout() {
    state.user = null;
    state.currentTask = null;
    state.currentReport = null;
    document.getElementById('authBar').style.display = 'flex';
    document.getElementById('userBar').style.display = 'none';
    document.getElementById('loginInput').value = '';
    document.getElementById('passwordInput').value = '';
    document.getElementById('authError').textContent = '';
    localStorage.removeItem('state');
    showPage('pageHome');
}

document.getElementById('passwordInput').addEventListener('keydown', e => {
    if (e.key === 'Enter') doLogin();
});

// ==========================
//  STUDENT — TASK LIST
// ==========================
async function loadStudentTasks() {
    showPage('pageStudentTasks');
    const wrap = document.getElementById('taskTableWrap');
    wrap.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка заданий…</div>';

    if (state.user.group_number) {
        document.getElementById('studentGroupLabel').textContent =
            'Группа: ' + state.user.group_number;
    }

    try {
        // GET /tasks?
        // Expected: [ { id, title, subject, teacher,
        //               report_status, report_grade, group_number } ]
        const tasks = await apiGet('/tasks');
        console.log('Loaded student tasks:', tasks);
        renderTaskTable(tasks, wrap);
    } catch (e) {
        console.error('Error loading student tasks:', e);
        wrap.innerHTML = `<div class="empty-state">
      <div class="es-icon">⚠</div>
      <p>Не удалось загрузить задания. Проверьте соединение.</p></div>`;
    }
}

function renderTaskTable(tasks, wrap) {
    if (!tasks.length) {
        wrap.innerHTML = `<div class="empty-state">
      <div class="es-icon">📋</div>
      <p>Заданий пока нет.</p></div>`;
        return;
    }
    wrap.innerHTML = `
    <table>
      <thead><tr>
        <th>#</th>
        <th>Название</th>
        <th>Предмет</th>
        <th>Преподаватель</th>
        <th>Статус</th>
        <th>Оценка</th>
      </tr></thead>
      <tbody>
        ${tasks.map(t => {
            const report = t.report || {};
            return `
          <tr onclick="openTask(${t.id})">
            <td>${t.id}</td>
            <td><strong>${escHtml(t.title)}</strong></td>
            <td>${escHtml(t.subject || '—')}</td>
            <td>${escHtml(t.teacher || '—')}</td>
            <td>${report.status ? badgeHtml(report.status) : '<span class="grade-none">нет ответа</span>'}</td>
            <td>${report.grade != null ? `<span class="grade-badge">${report.grade}</span>` : '<span class="grade-none">—</span>'}</td>
          </tr>`;
        }).join('')}
      </tbody>
    </table>`;
}

// ==========================
//  STUDENT — TASK DETAIL
// ==========================
async function openTask(taskId) {
    state.currentTask = { task_id: taskId };
    showPage('pageTaskDetail');

    const card = document.getElementById('taskDetailCard');
    card.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка…</div>';
    document.getElementById('reportText').value = '';
    document.getElementById('reportMsg').className = 'msg';

    try {
        // GET /task
        // Expected: { id, title, description, subject, teacher,
        //             group_number, report_id?, report_text?, report_status?, report_grade? }
        const d = await apiGet(`/tasks/${taskId}`);
        state.currentTask = d;
        console.log('Loaded task detail:', d);
        renderTaskDetail(d, card);
    } catch (e) {
        card.innerHTML = `<div class="empty-state"><p>Не удалось загрузить задание.</p></div>`;
    }
}

function renderTaskDetail(d, card) {
    const report = d.report || {};
    card.innerHTML = `
    <h3>${escHtml(d.title)}</h3>
    <div class="detail-meta">
      <span>📚 ${escHtml(d.subject || '—')}</span>
      <span>👤 ${escHtml(d.teacher || '—')}</span>
      <span>🎓 Группа: ${escHtml(d.group_number || '—')}</span>
      ${report.status ? `<span>${badgeHtml(report.status)}</span>` : ''}
      ${report.grade != null ? `<span><span class="grade-badge">${report.grade}</span></span>` : ''}
    </div>
    <div class="detail-body">${escHtml(d.description || '(описание отсутствует)')}</div>`;

    if (report.text) {
        document.getElementById('reportText').value = report.text;
    }

    const formWrap = document.getElementById('reportFormWrap');
    if (!report.status || report.status === 'REJECTED') {
        formWrap.style.display = '';
    } else {
        formWrap.style.display = 'none';
        let addBtn = document.getElementById('btnAddReport');
        if (addBtn) addBtn.style.display = 'none';
    }
}

async function submitReport() {
    const text = document.getElementById('reportText').value.trim();
    const btn = document.getElementById('btnSubmitReport');
    if (!text) { setMsg('reportMsg', 'Введите текст ответа.', 'err'); return; }

    btn.disabled = true;
    btn.textContent = 'Отправка…';
    try {
        if (isOfflineMode) {
            const st = offlineData.studentTasks.find(t => t.task_id === state.currentTask.task_id || t.task_id === state.currentTask.id);
            if (st) {
                st.report_text = text;
                st.report_status = 'SENT';
                st.report_grade = null;
            }
            setMsg('reportMsg', 'Ответ успешно отправлен! (тестовый режим)', 'ok');
            setTimeout(() => openTask(st.task_id), 700);
        } else {
            // POST /reports  { task_id, text }
            await apiPost('/reports', {
                task_id: state.currentTask.task_id || state.currentTask.id,
                text
            });
            setMsg('reportMsg', 'Ответ успешно отправлен!', 'ok');
            setTimeout(() => openTask(state.currentTask.task_id || state.currentTask.id), 700);
        }
    } catch (e) {
        setMsg('reportMsg', 'Ошибка при отправке. Попробуйте снова.', 'err');
    } finally {
        btn.disabled = false;
        btn.textContent = 'Отправить ответ';
    }
}


async function saveReportGrade() {
    const grade = document.getElementById('gradeInput').value;
    const status = document.getElementById('statusSelect').value;

    try {
        // POST /report/update  { report_id, grade, status }
        await apiPost(`/reports/${state.currentReport.id}`, {
            status,
            grade
        });
        setMsg('gradeMsg', 'Сохранено!', 'ok');
        const card = document.getElementById('reportDetailCard');
        const badgeEl = card.querySelector('.badge');
        if (badgeEl) {
            badgeEl.className = `badge badge-${status}`;
            badgeEl.textContent = statusLabel(status);
        }
    } catch (e) {
        setMsg('gradeMsg', 'Ошибка при сохранении.', 'err');
    }
}

// ==========================
//  OTHER FUNCTIONS
// ==========================
function escHtml(s) {
    if (s == null) return '';
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}
