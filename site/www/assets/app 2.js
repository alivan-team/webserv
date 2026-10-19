const $ = (s) => document.querySelector(s);
const setOut = (id, text, ok = true) => {
  const el = $(id);
  if (!el) return;
  el.textContent = text;
  el.className = ok ? 'status-good' : 'status-bad';
};

async function request(url, options = {}) {
  const response = await fetch(url, options);
  const text = await response.text();
  return { response, text };
}

async function refreshUploads() {
  const host = $('#upload-list');
  host.innerHTML = '<span class="muted">reading /uploads/ autoindex…</span>';
  try {
    const { response, text } = await request('/uploads/');
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const doc = new DOMParser().parseFromString(text, 'text/html');
    const links = [...doc.querySelectorAll('a')].filter(a => a.getAttribute('href'));
    host.innerHTML = '';
    if (!links.length) {
      host.innerHTML = '<span class="muted">No uploaded files yet.</span>';
      return;
    }
    links.forEach(link => {
      const href = link.getAttribute('href');
      const filename = href.split('/').filter(Boolean).pop();
      if (!filename) return;
      const row = document.createElement('div');
      row.className = 'file-row';
      row.innerHTML = `<a href="${href}" target="_blank" rel="noreferrer">${filename}</a><button type="button">DELETE</button>`;
      row.querySelector('button').addEventListener('click', async () => {
        const res = await fetch(href, { method: 'DELETE' });
        setOut('#out-delete', `DELETE ${href} → HTTP ${res.status} ${res.status === 204 ? 'No Content' : ''}`, res.status === 204);
        await refreshUploads();
      });
      host.appendChild(row);
    });
  } catch (err) {
    host.innerHTML = `<span class="status-bad">${err.message}</span>`;
  }
}

document.addEventListener('click', async (event) => {
  const button = event.target.closest('[data-test]');
  if (!button) return;
  const test = button.dataset.test;
  button.disabled = true;
  try {
    if (test === 'static') {
      const { response } = await request('/about/');
      setOut('#out-static', `GET /about/ → HTTP ${response.status}`, response.status === 200);
    }
    if (test === 'cgi-get') {
      const value = encodeURIComponent($('#cgi-name').value);
      const { response, text } = await request(`/cgi-bin/echo.py?name=${value}&mode=get`);
      setOut('#out-cgi', `GET CGI → HTTP ${response.status}\n${text.slice(0, 900)}`, response.status === 200);
    }
    if (test === 'cgi-post') {
      const value = $('#cgi-name').value;
      const { response, text } = await request('/cgi-bin/echo.py?mode=post', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain', 'X-Evaluation-Demo': 'webserv' },
        body: `Hello from the browser. Name=${value}`
      });
      setOut('#out-cgi', `POST CGI → HTTP ${response.status}\n${text.slice(0, 900)}`, response.status === 200);
    }
    if (test === 'cgi-js-get') {
      const value = encodeURIComponent($('#cgi-name').value);
      const { response, text } = await request(`/cgi-bin/testjs.js?name=${value}&mode=get`);
      setOut('#out-cgi', `GET CGI → HTTP ${response.status}\n${text.slice(0, 900)}`, response.status === 200);
    }
    if (test === 'upload') {
      const fileInput = $('#upload-file');
      if (!fileInput.files.length) {
        setOut('#out-upload', 'Choose a file first.', false);
      } else {
        const data = new FormData();
        data.append('file', fileInput.files[0]);
        const response = await fetch('/upload', { method: 'POST', body: data });
        const location = response.headers.get('Location') || '(no Location header)';
        setOut('#out-upload', `POST /upload → HTTP ${response.status}\nLocation: ${location}`, response.status === 201);
        await refreshUploads();
      }
    }
    if (test === 'refresh-uploads') await refreshUploads();
    if (test === '405') {
      const { response } = await request('/get-only', { method: 'POST', body: 'not allowed' });
      setOut('#out-405', `POST /get-only → HTTP ${response.status}\nAllow: ${response.headers.get('Allow') || '(none)'}`, response.status === 405);
    }
    if (test === '404') {
      const { response } = await request('/definitely-not-here');
      setOut('#out-404', `GET /definitely-not-here → HTTP ${response.status}`, response.status === 404);
    }
    if (test === '413') {
      const tooLarge = 'X'.repeat(150000);
      const response = await fetch('/upload', {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: tooLarge
      });
      setOut('#out-413', `POST 150000 bytes → HTTP ${response.status}`, response.status === 413);
    }
    if (test === '501') {
      const { response } = await request('/', { method: 'PUT', body: 'unsupported' });
      setOut('#out-501', `PUT / → HTTP ${response.status}`, response.status === 501);
    }
  } catch (err) {
    const map = {static:'#out-static','cgi-get':'#out-cgi','cgi-post':'#out-cgi',upload:'#out-upload','405':'#out-405','404':'#out-404','413':'#out-413','501':'#out-501'};
    if (map[test]) setOut(map[test], `Request failed: ${err.message}`, false);
  } finally {
    button.disabled = false;
  }
});

refreshUploads();
