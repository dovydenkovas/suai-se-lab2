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
    else loadTeacherReports();
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

async function apiGet(path) {
    const r = await fetch('/api' + path, {
        headers: {
            'Authorization': `Bearer ${state.token}`
        }
    });
    if (!r.ok) throw new Error(`HTTP ${r.status}`);
    return r.json();
}

async function apiPost(path, data) {
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
    else{
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
        state.user.full_name + (state.user.role === 'student' ? ' (студент)' : ' (преподаватель)');

    if (state.user.role === 'student') loadStudentTasks();
    else loadTeacherReports();
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
        // GET /tasks?student_id=X
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
            <td>${t.report_grade != null
            ? `<span class="grade-badge">${t.report_grade}</span>`
            : '<span class="grade-none">—</span>'}</td>
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
}

async function submitReport() {
    const text = document.getElementById('reportText').value.trim();
    const btn = document.getElementById('btnSubmitReport');
    if (!text) { setMsg('reportMsg', 'Введите текст ответа.', 'err'); return; }

    btn.disabled = true;
    btn.textContent = 'Отправка…';
    try {
        // POST /report  { task_id, student_id, text }
        await apiPost('/reports', {
            task_id: state.currentTask.id,
            text
        });
        setMsg('reportMsg', 'Ответ успешно отправлен!', 'ok');
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
