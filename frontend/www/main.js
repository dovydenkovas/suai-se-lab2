// ═══════════════════════════════════════════════════
//  CONFIG  — Change CGI_BASE to your actual endpoint
// ═══════════════════════════════════════════════════
const CGI_BASE = '/cgi-bin';  // e.g. 'http://your-server/cgi-bin'

// ── App state ──
const state = {
    user: null,
    token: null,
    currentTask: null,
    currentReport: null,
};

// ═══════════════════════════════════════════════════
//  UTILITIES
// ═══════════════════════════════════════════════════
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



// ====== MOCK DATA FOR OFFLINE MODE ======
const mock = {
    teacherTasks: [
        { task_id: 1, title: 'Лабораторная 1', subject_name: 'Основы программирования', group_number: 'Б01-001', description: 'Сделать задание 1.' },
        { task_id: 2, title: 'Лабораторная 2', subject_name: 'Основы программирования', group_number: 'Б01-001', description: 'Сделать задание 2.' }
    ],
    studentTasks: [
        { task_id: 1, title: 'Лабораторная 1', subject_name: 'Основы программирования', teacher_name: 'Иван Иванов', report_status: 'SENT', report_grade: null, group_number: 'Б01-001', report_text: 'Мой ответ на ЛР1' },
        { task_id: 2, title: 'Лабораторная 2', subject_name: 'Основы программирования', teacher_name: 'Иван Иванов', report_status: null, report_grade: null, group_number: 'Б01-001', report_text: '' }
    ]
};
let mockTaskId = 3;

// Переключатель: true — использовать mock, false — реальное API
let USE_MOCK = true;

async function apiGet(path) {
    if (USE_MOCK) {
        // OFFLINE MOCKS
        if (path === '/teacher/tasks') {
            return JSON.parse(JSON.stringify(mock.teacherTasks));
        }
        if (path.startsWith('/teacher/tasks/')) {
            const id = parseInt(path.split('/').pop());
            return JSON.parse(JSON.stringify(mock.teacherTasks.find(t => t.task_id === id)));
        }
        if (path === '/tasks') {
            return JSON.parse(JSON.stringify(mock.studentTasks));
        }
        if (path.startsWith('/tasks/')) {
            const id = parseInt(path.split('/').pop());
            // Для детального задания студенту
            const t = mock.studentTasks.find(t => t.task_id === id);
            if (!t) throw new Error('not found');
            return Object.assign({}, t, { description: mock.teacherTasks.find(tt => tt.task_id === id)?.description });
        }
        if (path.startsWith('/reports')) {
            // Мок для отчётов преподавателя
            return [];
        }
        throw new Error('OFFLINE: Not implemented for ' + path);
    } else {
        // REAL API
        const r = await fetch('/api' + path, {
            headers: {
                'Authorization': `Bearer ${state.token}`
            }
        });
        if (!r.ok) throw new Error(`HTTP ${r.status}`);
        return r.json();
    }
}

async function apiPost(path, data) {
    if (USE_MOCK) {
        // OFFLINE MOCKS
        if (path === '/teacher/tasks') {
            // create
            const newTask = {
                task_id: mockTaskId++,
                title: data.title,
                description: data.description,
                group_number: data.group_number,
                subject_name: data.subject_name
            };
            mock.teacherTasks.push(newTask);
            mock.studentTasks.push({
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
            const t = mock.teacherTasks.find(t => t.task_id === id);
            if (t) {
                t.title = data.title;
                t.description = data.description;
                t.group_number = data.group_number;
                t.subject_name = data.subject_name;
                // sync for student
                const st = mock.studentTasks.find(st => st.task_id === id);
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
            mock.teacherTasks = mock.teacherTasks.filter(t => t.task_id !== id);
            mock.studentTasks = mock.studentTasks.filter(t => t.task_id !== id);
            return { ok: true };
        }
        throw new Error('OFFLINE: Not implemented for ' + path);
    } else {
        // REAL API
        const r = await fetch('/api' + path, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': `Bearer ${state.token}`
            },
            body: JSON.stringify(data),
        });
        if (!r.ok) throw new Error(`HTTP ${r.status}`);
        return r.json();
    }
}

// ═══════════════════════════════════════════════════
//  AUTH
// ═══════════════════════════════════════════════════
async function doLogin() {
    const login = document.getElementById('loginInput').value.trim();
    const password = document.getElementById('passwordInput').value;

    if (login == 1 && password == 1) {
        // Quick test login for teacher
        state.user = { id: 1, full_name: 'Иван Иванов', role: 'teacher' };
    }
    else if (login == 2 && password == 2) {
        // Quick test login for student
        state.user = { id: 2, full_name: 'Пётр Петров', role: 'student', group_number: 'Б01-001' };
    }
    else {
        const data = await apiPost('/auth/login', { login, password });
        state.user = data.user;
        state.token = data.token;
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
    else loadTeacherTasks();
}
// ═══════════════════════════════════════════════════
//  TEACHER — TASKS PAGE
// ═══════════════════════════════════════════════════
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
                <th>Действия</th>
            </tr></thead>
            <tbody>
                ${tasks.map(t => `
                    <tr>
                        <td>${t.task_id}</td>
                        <td><a href="#" onclick="showTaskAnswers(${t.task_id});return false;">${escHtml(t.title)}</a></td>
                        <td>${escHtml(t.group_number || '—')}</td>
                        <td>${escHtml(t.subject_name || '—')}</td>
                        <td>
                            <button class="btn-small" onclick="editTask(${t.task_id})">Редактировать</button>
                        </td>
                    </tr>`).join('')}
            </tbody>
        </table>`;
}

// Показывает ответы студентов на задание для преподавателя
async function showTaskAnswers(taskId) {
    showPage('pageTaskDetail');
    const card = document.getElementById('taskDetailCard');
    card.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка…</div>';
    let task = {};
    let answers = [];
    if (USE_MOCK) {
        task = (await apiGet('/teacher/tasks/' + taskId)) || {};
        answers = mock.studentTasks.filter(s => s.task_id === taskId && s.report_status);
    } else {
        task = await apiGet('/teacher/tasks/' + taskId);
        answers = await apiGet(`/reports?task_id=${taskId}`);
    }
    // Сохраняем taskId для возврата
    showTaskAnswers._lastTaskId = taskId;
    // Формируем HTML с выпадающим меню для статуса
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
          <div class="answer-block" style="margin-bottom:1.5rem;">
            <div style="font-size:0.97rem;"><b>${escHtml(a.student_name || a.teacher_name || 'Студент')}</b> ${a.status ? badgeHtml(a.status) : (a.report_status ? badgeHtml(a.report_status) : '')}</div>
            <div style="margin:0.5rem 0 0.5rem 0; color:var(--ink-soft); white-space:pre-wrap;">${escHtml(a.text || a.report_text || '(нет текста)')}</div>
            ${(a.grade != null ? `<span class=\"grade-badge\">${a.grade}</span>` : (a.report_grade != null ? `<span class=\"grade-badge\">${a.report_grade}</span>` : ''))}
            <div style="margin-top:0.5rem;">
              <label for="statusSelect_${idx}" style="font-size:0.88rem; color:var(--ink-faint); margin-right:0.5rem;">Статус:</label>
              <select id="statusSelect_${idx}" data-answer-idx="${idx}" onchange="onChangeAnswerStatus(${taskId}, ${idx}, this.value)">
                <option value="SENT" ${(a.status === 'SENT' || a.status === undefined || a.status === null) ? 'selected' : ''}>Отправлен</option>
                <option value="ACCEPTED" ${a.status === 'ACCEPTED' ? 'selected' : ''}>Принят</option>
                <option value="REJECTED" ${a.status === 'REJECTED' ? 'selected' : ''}>Не принят</option>
              </select>
            </div>
          </div>
        `).join('')}
    `;
    // Скрыть форму ответа для преподавателя
    document.getElementById('reportFormWrap').style.display = 'none';
}

// Обработчик изменения статуса ответа
window.onChangeAnswerStatus = async function (taskId, idx, newStatus) {
    if (USE_MOCK) {
        // В mock-режиме меняем статус в mock.studentTasks
        const answers = mock.studentTasks.filter(s => s.task_id === taskId && s.report_status);
        if (answers[idx]) {
            answers[idx].report_status = newStatus;
        }
        // Перерисовать
        showTaskAnswers(taskId);
    } else {
        // В реальном API — отправить POST /api/reports/{report_id} {status}
        // Нужно знать report_id, предполагаем что answers[idx].report_id есть
        const answers = await apiGet(`/reports?task_id=${taskId}`);
        const answer = answers[idx];
        if (answer && answer.report_id) {
            await apiPost(`/reports/${answer.report_id}`, { status: newStatus });
            // Перерисовать
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
    document.getElementById('btnDeleteTask').style.display = 'none';
    document.getElementById('taskEditorMsg').className = 'msg';
    document.getElementById('taskEditorMsg').textContent = '';
    showPage('pageTaskEditor');
}

async function editTask(taskId) {
    editingTaskId = taskId;
    showPage('pageTaskEditor');
    document.getElementById('taskEditorTitle').textContent = 'Редактировать задание';
    document.getElementById('btnDeleteTask').style.display = 'inline-block';
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
            await apiPost(`/teacher/tasks/${editingTaskId}`, { title, description, group_number, subject_name });
            setMsg('taskEditorMsg', 'Задание обновлено!', 'ok');
        } else {
            // POST /teacher/tasks (create)
            await apiPost('/teacher/tasks', { title, description, group_number, subject_name });
            setMsg('taskEditorMsg', 'Задание добавлено!', 'ok');
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
        // POST /teacher/tasks/:id/delete
        await apiPost(`/teacher/tasks/${editingTaskId}/delete`, {});
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
    showPage('pageHome');
}

// Enter key on password field
document.getElementById('passwordInput').addEventListener('keydown', e => {
    if (e.key === 'Enter') doLogin();
});

// ═══════════════════════════════════════════════════
//  STUDENT — TASK LIST
// ═══════════════════════════════════════════════════
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
        // Expected: [ { task_id, title, subject_name, teacher_name,
        //               report_status, report_grade, group_number } ]
        const tasks = await apiGet('/tasks');
        renderTaskTable(tasks, wrap);
    } catch (e) {
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
        ${tasks.map(t => `
          <tr onclick="openTask(${t.task_id})">
            <td>${t.task_id}</td>
            <td><strong>${escHtml(t.title)}</strong></td>
            <td>${escHtml(t.subject_name || '—')}</td>
            <td>${escHtml(t.teacher_name || '—')}</td>
            <td>${t.report_status ? badgeHtml(t.report_status) : '<span class="grade-none">нет ответа</span>'}</td>
            <td>${t.report_grade != null ? `<span class="grade-badge">${t.report_grade}</span>` : '<span class="grade-none">—</span>'}</td>
          </tr>`).join('')}
      </tbody>
    </table>`;
}

// ═══════════════════════════════════════════════════
//  STUDENT — TASK DETAIL
// ═══════════════════════════════════════════════════
async function openTask(taskId) {
    state.currentTask = { task_id: taskId };
    showPage('pageTaskDetail');

    const card = document.getElementById('taskDetailCard');
    card.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка…</div>';
    document.getElementById('reportText').value = '';
    document.getElementById('reportMsg').className = 'msg';

    try {
        // GET /task
        // Expected: { task_id, title, description, subject_name, teacher_name,
        //             group_number, report_id?, report_text?, report_status?, report_grade? }
        const d = await apiGet(`/tasks/${taskId}`);
        state.currentTask = d;
        renderTaskDetail(d, card);
    } catch (e) {
        card.innerHTML = `<div class="empty-state"><p>Не удалось загрузить задание.</p></div>`;
    }
}

function renderTaskDetail(d, card) {
    card.innerHTML = `
    <h3>${escHtml(d.title)}</h3>
    <div class="detail-meta">
      <span>📚 ${escHtml(d.subject_name || '—')}</span>
      <span>👤 ${escHtml(d.teacher_name || '—')}</span>
      <span>🎓 Группа: ${escHtml(d.group_number || '—')}</span>
      ${d.report_status ? `<span>${badgeHtml(d.report_status)}</span>` : ''}
      ${d.report_grade != null ? `<span><span class="grade-badge">${d.report_grade}</span></span>` : ''}
    </div>
    <div class="detail-body">${escHtml(d.description || '(описание отсутствует)')}</div>`;

    // Pre-fill existing answer
    if (d.report_text) {
        document.getElementById('reportText').value = d.report_text;
    }

    // Показать/скрыть форму ответа и кнопку "добавить ответ"
    const formWrap = document.getElementById('reportFormWrap');
    const btnSubmit = document.getElementById('btnSubmitReport');
    // Если нет ответа или ответ отклонён — показать кнопку "добавить ответ"
    if (!d.report_status || d.report_status === 'REJECTED') {
        formWrap.style.display = '';
        // Добавить кнопку, если её нет
        let addBtn = document.getElementById('btnAddReport');
        if (!addBtn) {
            addBtn = document.createElement('button');
            addBtn.id = 'btnAddReport';
            addBtn.className = 'btn-primary';
            addBtn.style = 'margin-top:1rem;';
            addBtn.textContent = 'Добавить ответ';
            addBtn.onclick = function() {
                formWrap.scrollIntoView({behavior: 'smooth'});
                document.getElementById('reportText').focus();
            };
            formWrap.parentNode.insertBefore(addBtn, formWrap);
        } else {
            addBtn.style.display = '';
        }
    } else {
        // Если ответ уже отправлен и не отклонён — скрыть форму и кнопку
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
        if (USE_MOCK) {
            // Найти задание студента и обновить ответ
            const st = mock.studentTasks.find(t => t.task_id === state.currentTask.task_id || t.task_id === state.currentTask.id);
            if (st) {
                st.report_text = text;
                st.report_status = 'SENT';
                st.report_grade = null;
            }
            setMsg('reportMsg', 'Ответ успешно отправлен! (тестовый режим)', 'ok');
            // Перерисовать детальную страницу задания
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

// ═══════════════════════════════════════════════════
//  TEACHER — REPORT LIST
// ═══════════════════════════════════════════════════
async function loadTeacherReports() {
    showPage('pageTeacherReports');
    const wrap = document.getElementById('reportTableWrap');
    wrap.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка отчётов…</div>';
    document.getElementById('teacherNameLabel').textContent = state.user.full_name;

    try {
        // GET /reports
        // Expected: [ { report_id, task_id, task_title, subject_name,
        //               student_name, group_number, status, grade } ]
        const reports = await apiGet('/reports');
        renderReportTable(reports, wrap);
    } catch (e) {
        wrap.innerHTML = `<div class="empty-state">
      <div class="es-icon">⚠</div>
      <p>Не удалось загрузить отчёты.</p></div>`;
    }
}

function renderReportTable(reports, wrap) {
    if (!reports.length) {
        wrap.innerHTML = `<div class="empty-state">
      <div class="es-icon">📂</div>
      <p>Отчётов пока нет.</p></div>`;
        return;
    }
    wrap.innerHTML = `
    <table>
      <thead><tr>
        <th>#</th>
        <th>Студент</th>
        <th>Группа</th>
        <th>Задание</th>
        <th>Предмет</th>
        <th>Статус</th>
        <th>Оценка</th>
      </tr></thead>
      <tbody>
        ${reports.map(r => `
          <tr onclick="openReport(${r.report_id})">
            <td>${r.report_id}</td>
            <td><strong>${escHtml(r.student_name || '—')}</strong></td>
            <td>${escHtml(r.group_number || '—')}</td>
            <td>${escHtml(r.task_title || '—')}</td>
            <td>${escHtml(r.subject_name || '—')}</td>
            <td>${badgeHtml(r.status)}</td>
            <td>${r.grade != null
            ? `<span class="grade-badge">${r.grade}</span>`
            : '<span class="grade-none">—</span>'}</td>
          </tr>`).join('')}
      </tbody>
    </table>`;
}

// ═══════════════════════════════════════════════════
//  TEACHER — REPORT DETAIL
// ═══════════════════════════════════════════════════
async function openReport(reportId) {
    state.currentReport = { report_id: reportId };
    showPage('pageReportDetail');

    const card = document.getElementById('reportDetailCard');
    card.innerHTML = '<div class="loading"><span class="spinner"></span> Загрузка…</div>';
    document.getElementById('gradeMsg').className = 'msg';

    try {
        // GET /report?report_id=X
        // Expected: { report_id, task_id, task_title, task_description,
        //             subject_name, student_name, group_number,
        //             text, status, grade }
        const d = await apiGet(`/reports/${reportId}`);
        state.currentReport = d;
        renderReportDetail(d, card);
    } catch (e) {
        card.innerHTML = `<div class="empty-state"><p>Не удалось загрузить отчёт.</p></div>`;
    }
}

function renderReportDetail(d, card) {
    card.innerHTML = `
    <h3>${escHtml(d.task_title || 'Отчёт #' + d.report_id)}</h3>
    <div class="detail-meta">
      <span>📚 ${escHtml(d.subject_name || '—')}</span>
      <span>👤 ${escHtml(d.student_name || '—')}</span>
      <span>🎓 ${escHtml(d.group_number || '—')}</span>
      <span>${badgeHtml(d.status)}</span>
      ${d.grade != null ? `<span>Оценка: <span class="grade-badge">${d.grade}</span></span>` : ''}
    </div>
    ${d.task_description ? `
      <div style="font-size:0.88rem; color:var(--ink-faint); margin-bottom:0.5rem; font-weight:600; letter-spacing:0.03em; text-transform:uppercase;">Текст задания</div>
      <div style="font-size:0.93rem; color:var(--ink-soft); background:var(--bg); border-radius:var(--radius); padding:0.85rem 1rem; margin-bottom:1rem; border:1px solid var(--border); line-height:1.65; white-space:pre-wrap;">${escHtml(d.task_description)}</div>
    ` : ''}
    <div style="font-size:0.88rem; color:var(--ink-faint); margin-bottom:0.5rem; font-weight:600; letter-spacing:0.03em; text-transform:uppercase;">Ответ студента</div>
    <div class="detail-body">${escHtml(d.text || '(ответ отсутствует)')}</div>`;

    // Pre-fill form
    document.getElementById('gradeInput').value = d.grade ?? '';
    document.getElementById('statusSelect').value = d.status ?? 'SENT';
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
        setMsg('gradeMsg', 'Сохранено успешно!', 'ok');
        // Update displayed badge
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

// ═══════════════════════════════════════════════════
//  OTHER FUNCTIONS
// ═══════════════════════════════════════════════════
function escHtml(s) {
    if (s == null) return '';
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}
