const state = {
    theme: localStorage.getItem('dc_theme') || 'light',
    authTab: 'login',
    user: null,
    isAdmin: false,
    userTab: 'fleet',
    adminTab: 'analytics',
    vehicles: [],
    category: 'All',
    search: '',
    sort: 'default',
    lockoutQuestion: '',
    booking: null
};

document.body.setAttribute('data-theme', state.theme);

function toast(msg, type = 'ok') {
    const el = document.getElementById('toast');
    el.textContent = msg;
    el.className = `toast show ${type === 'error' ? 'error' : ''}`;
    clearTimeout(toast._t);
    toast._t = setTimeout(() => el.classList.remove('show'), 3600);
}

function toggleTheme() {
    state.theme = state.theme === 'dark' ? 'light' : 'dark';
    localStorage.setItem('dc_theme', state.theme);
    document.body.setAttribute('data-theme', state.theme);
    renderApp();
}

function toForm(obj) {
    return Object.entries(obj)
        .map(([k, v]) => `${encodeURIComponent(k)}=${encodeURIComponent(v)}`)
        .join('&');
}

async function postForm(url, data) {
    const res = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: toForm(data)
    });
    return res.json();
}

function vehicleIcon(type) {
    const t = (type || '').toLowerCase();
    if (t.includes('bike')) return '🏍️';
    if (t.includes('suv')) return '🚙';
    return '🚘';
}

function statusTag(status) {
    if (status === 'Available') return '<span class="tag tag-ok">Available</span>';
    if (status === 'Under Maintenance') return '<span class="tag tag-warn">Under Maintenance</span>';
    return `<span class="tag tag-bad">${status}</span>`;
}

// --- AUTH VIEW (JS Rendered) ---
function renderAuth() {
    const authEl = document.getElementById('authView');
    const appEl = document.getElementById('appView');
    appEl.classList.add('hidden');
    authEl.classList.remove('hidden');

    authEl.innerHTML = `
        <section class="auth-editorial">
            <div class="brand-mark">DriveCore <span class="brand-badge">C-ENGINE</span></div>
            <div>
                <h1>Reserve vehicles with zero bloat.</h1>
                <p>Native C Winsock server paired with file-backed persistence, instant trip pricing, multi-gateway checkout, and live audit logs.</p>
                <div class="spec-list">
                    <span class="spec-item">Winsock HTTP Server</span>
                    <span class="spec-item">bKash / Nagad / Card / COD</span>
                    <span class="spec-item">Audit Log Trail</span>
                    <span class="spec-item">CSV Export</span>
                </div>
            </div>
            <div class="mono" style="font-size:0.78rem; color:var(--muted);">localhost:8080 • File Storage Active</div>
        </section>
        <section class="auth-form-col">
            <div class="auth-box">
                <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:1rem;">
                    <div class="tabs-bar" style="margin-bottom:0;">
                        <button type="button" class="tab-btn ${state.authTab === 'login' ? 'active' : ''}" onclick="setAuthTab('login')">Sign In</button>
                        <button type="button" class="tab-btn ${state.authTab === 'register' ? 'active' : ''}" onclick="setAuthTab('register')">Register</button>
                    </div>
                    <button type="button" class="btn btn-ghost" style="padding:7px 11px; font-size:0.78rem;" onclick="toggleTheme()">${state.theme === 'dark' ? '☀️' : '🌙'}</button>
                </div>
                ${state.authTab === 'login' ? renderLoginForm() : renderRegisterForm()}
            </div>
        </section>
    `;
}

function setAuthTab(tab) {
    state.authTab = tab;
    state.lockoutQuestion = '';
    renderAuth();
}

function renderLoginForm() {
    return `
        <h2 style="font-size:1.5rem; font-weight:800; margin-bottom:4px;">Welcome back</h2>
        <p style="color:var(--muted); font-size:0.88rem; margin-bottom:1.35rem;">Sign in to book rides or open AdminOS.</p>
        <form onsubmit="submitLogin(event)">
            <div class="field">
                <label>Email</label>
                <input type="email" id="lEmail" required placeholder="you@example.com">
            </div>
            <div class="field">
                <label>Password</label>
                <input type="password" id="lPass" required placeholder="Enter password">
            </div>
            <button class="btn btn-brand" type="submit">Continue</button>
        </form>
        ${state.lockoutQuestion ? `
            <div class="lockout-box">
                <div style="font-weight:800; font-size:0.88rem; color:var(--bad); margin-bottom:4px;">Account Locked — Security Reset</div>
                <div class="mono" style="font-size:0.82rem; margin-bottom:10px; background:var(--surface); padding:8px; border-radius:8px;">${state.lockoutQuestion}</div>
                <form onsubmit="submitReset(event)">
                    <div class="field"><label>Security Answer</label><input type="text" id="rAns" required></div>
                    <div class="row-2">
                        <div class="field"><label>New Password</label><input type="password" id="rPass1" required></div>
                        <div class="field"><label>Confirm</label><input type="password" id="rPass2" required></div>
                    </div>
                    <button class="btn btn-brand" style="background:var(--bad); color:#fff;" type="submit">Unlock & Reset</button>
                </form>
            </div>
        ` : ''}
    `;
}

function renderRegisterForm() {
    return `
        <h2 style="font-size:1.5rem; font-weight:800; margin-bottom:4px;">Create account</h2>
        <p style="color:var(--muted); font-size:0.88rem; margin-bottom:1.35rem;">6+ chars with letter, number & symbol.</p>
        <form onsubmit="submitRegister(event)">
            <div class="row-2">
                <div class="field"><label>Full Name</label><input type="text" id="rName" required></div>
                <div class="field"><label>Phone</label><input type="text" id="rPhone" required></div>
            </div>
            <div class="field"><label>Email</label><input type="email" id="rEmail" required></div>
            <div class="field"><label>Password</label><input type="password" id="rPass" required></div>
            <div class="row-2">
                <div class="field"><label>Security Question</label><input type="text" id="rSecQ" required placeholder="First school?"></div>
                <div class="field"><label>Answer</label><input type="text" id="rSecA" required></div>
            </div>
            <button class="btn btn-brand" type="submit">Create Account</button>
        </form>
    `;
}

async function submitLogin(e) {
    e.preventDefault();
    const email = document.getElementById('lEmail').value.trim();
    const password = document.getElementById('lPass').value;

    if (email.toLowerCase() === 'admin@rental.com') {
        const res = await postForm('/api/admin-login', { email, password });
        if (res.status === 'success') {
            state.isAdmin = true;
            state.adminTab = 'analytics';
            toast('Admin console unlocked');
            renderApp();
        } else {
            toast(res.message || 'Invalid admin credentials', 'error');
        }
        return;
    }

    const res = await postForm('/api/login', { email, password });
    if (res.status === 'success') {
        state.user = res.user;
        state.isAdmin = false;
        state.userTab = 'fleet';
        toast(`Signed in as ${state.user.name}`);
        renderApp();
    } else if (res.status === 'locked') {
        state.lockoutQuestion = res.sec_q;
        toast(res.message, 'error');
        renderAuth();
    } else {
        toast(res.message, 'error');
    }
}

async function submitRegister(e) {
    e.preventDefault();
    const res = await postForm('/api/register', {
        name: document.getElementById('rName').value.trim(),
        phone: document.getElementById('rPhone').value.trim(),
        email: document.getElementById('rEmail').value.trim(),
        password: document.getElementById('rPass').value,
        sec_q: document.getElementById('rSecQ').value.trim(),
        sec_a: document.getElementById('rSecA').value.trim()
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        state.authTab = 'login';
        renderAuth();
    }
}

async function submitReset(e) {
    e.preventDefault();
    const res = await postForm('/api/reset-password', {
        email: document.getElementById('lEmail').value.trim(),
        sec_a: document.getElementById('rAns').value.trim(),
        new_pass: document.getElementById('rPass1').value,
        confirm_pass: document.getElementById('rPass2').value
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        state.lockoutQuestion = '';
        renderAuth();
    }
}

function logout() {
    state.user = null;
    state.isAdmin = false;
    toast('Signed out');
    renderAuth();
}

// --- APP SHELL (JS Rendered) ---
function renderApp() {
    document.getElementById('authView').classList.add('hidden');
    const appEl = document.getElementById('appView');
    appEl.classList.remove('hidden');

    const navItems = state.isAdmin
        ? [
            ['analytics', 'Overview'],
            ['vehicles', 'Vehicles'],
            ['users', 'Customers'],
            ['add', 'Add Vehicle'],
            ['logs', 'Audit Log']
        ]
        : [
            ['fleet', 'Fleet'],
            ['bookings', 'My Rentals'],
            ['profile', 'Profile']
        ];

    const activeTab = state.isAdmin ? state.adminTab : state.userTab;

    appEl.innerHTML = `
        <header class="topbar">
            <div style="display:flex; align-items:center; gap:20px; flex-wrap:wrap;">
                <div class="brand-mark">${state.isAdmin ? 'AdminOS' : 'DriveCore'} <span class="brand-badge">${state.isAdmin ? 'ROOT' : 'FLEET'}</span></div>
                <nav class="subnav">
                    ${navItems.map(([key, label]) => `
                        <button type="button" class="subnav-btn ${activeTab === key ? 'active' : ''}" onclick="switchSection('${key}')">${label}</button>
                    `).join('')}
                </nav>
            </div>
            <div class="topbar-right">
                <span class="chip-user">${state.isAdmin ? 'admin@rental.com' : state.user.name}</span>
                <button type="button" class="btn btn-ghost" style="padding:7px 11px; font-size:0.8rem;" onclick="toggleTheme()">${state.theme === 'dark' ? '☀️' : '🌙'}</button>
                <button type="button" class="btn btn-soft-bad" onclick="logout()">Sign Out</button>
            </div>
        </header>
        <div class="content" id="workspaceRoot"></div>
    `;

    if (state.isAdmin) loadAdminSection();
    else loadUserSection();
}

function switchSection(key) {
    if (state.isAdmin) {
        state.adminTab = key;
        loadAdminSection();
    } else {
        state.userTab = key;
        loadUserSection();
    }
}

// --- USER WORKSPACE ---
async function loadUserSection() {
    const root = document.getElementById('workspaceRoot');
    const vRes = await fetch('/api/vehicles');
    const vData = await vRes.json();
    state.vehicles = vData.vehicles || [];

    const bRes = await postForm('/api/rentals/user', { email: state.user.email });
    const rentals = bRes.rentals || [];
    const availCount = state.vehicles.filter(v => v.status === 'Available').length;
    const activeCount = rentals.filter(r => r.status === 'Active').length;

    const kpiHtml = `
        <div class="metrics-strip">
            <div class="metric-box"><span>Available Vehicles</span><strong class="mono">${availCount}</strong></div>
            <div class="metric-box"><span>Active Bookings</span><strong class="mono" style="color:var(--brand);">${activeCount}</strong></div>
            <div class="metric-box"><span>Total Trips</span><strong class="mono">${rentals.length}</strong></div>
        </div>
    `;

    if (state.userTab === 'fleet') {
        root.innerHTML = kpiHtml + renderFleetControls() + `<div class="cards-grid" id="fleetCards"></div>`;
        renderFleetCards();
    } else if (state.userTab === 'bookings') {
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>My Rentals & Payments</h2></div>
                <table>
                    <thead><tr><th>ID</th><th>Vehicle</th><th>Period</th><th>Total</th><th>Gateway</th><th>Status</th><th>Action</th></tr></thead>
                    <tbody>
                        ${rentals.length ? rentals.map(r => `
                            <tr>
                                <td class="mono"><strong>${r.rental_id}</strong></td>
                                <td>${r.vehicle_name} <span class="mono" style="color:var(--muted);">(${r.vehicle_id})</span></td>
                                <td class="mono">${r.start_date} → ${r.end_date}</td>
                                <td class="mono"><strong>${r.total_cost.toFixed(0)} BDT</strong></td>
                                <td>${r.payment_method}</td>
                                <td>${statusTag(r.status)}</td>
                                <td style="display:flex; gap:6px;">
                                    ${r.status === 'Active' ? `
                                        <button class="btn btn-soft-ok" onclick="returnRental('${r.rental_id}','${r.vehicle_id}')">Return</button>
                                        <button class="btn btn-soft-bad" onclick="cancelRental('${r.rental_id}','${r.vehicle_id}')">Cancel</button>
                                    ` : '<span style="color:var(--muted); font-size:0.8rem;">—</span>'}
                                </td>
                            </tr>
                        `).join('') : '<tr><td colspan="7" style="color:var(--muted);">No rentals yet.</td></tr>'}
                    </tbody>
                </table>
            </div>
        `;
    } else if (state.userTab === 'profile') {
        root.innerHTML = kpiHtml + `
            <div class="panel" style="max-width:620px;">
                <div class="panel-head"><h2>Account & Password</h2></div>
                <form onsubmit="submitProfile(event)">
                    <div class="row-2">
                        <div class="field"><label>Name</label><input type="text" id="pName" value="${state.user.name}" required></div>
                        <div class="field"><label>Phone</label><input type="text" id="pPhone" value="${state.user.phone}" required></div>
                    </div>
                    <div class="row-2">
                        <div class="field"><label>Email</label><input type="email" value="${state.user.email}" disabled></div>
                        <div class="field"><label>New Password (optional)</label><input type="password" id="pNewPass" placeholder="Leave empty to keep"></div>
                    </div>
                    <div class="field"><label>Current Password (required)</label><input type="password" id="pCurPass" required></div>
                    <button class="btn btn-brand" style="width:auto;" type="submit">Update Profile</button>
                </form>
            </div>
        `;
    }
}

function renderFleetControls() {
    const cats = ['All', 'Sedan', 'SUV', 'Bike'];
    return `
        <div class="controls-bar">
            <div class="cat-group">
                ${cats.map(c => `<button type="button" class="cat-pill ${state.category === c ? 'active' : ''}" onclick="setCat('${c}')">${c}</button>`).join('')}
            </div>
            <div class="search-sort">
                <input type="search" placeholder="Search model or ID..." value="${state.search}" oninput="setSearch(this.value)">
                <select onchange="setSort(this.value)">
                    <option value="default" ${state.sort === 'default' ? 'selected' : ''}>Sort: Default</option>
                    <option value="asc" ${state.sort === 'asc' ? 'selected' : ''}>Rate: Low → High</option>
                    <option value="desc" ${state.sort === 'desc' ? 'selected' : ''}>Rate: High → Low</option>
                </select>
            </div>
        </div>
    `;
}

function setCat(c) { state.category = c; loadUserSection(); }
function setSearch(q) { state.search = q.toLowerCase(); renderFleetCards(); }
function setSort(s) { state.sort = s; renderFleetCards(); }

function renderFleetCards() {
    const grid = document.getElementById('fleetCards');
    if (!grid) return;

    let list = state.vehicles.filter(v => v.status === 'Available');
    if (state.category !== 'All') {
        list = list.filter(v => v.type.toLowerCase() === state.category.toLowerCase());
    }
    if (state.search) {
        list = list.filter(v => v.name.toLowerCase().includes(state.search) || v.id.toLowerCase().includes(state.search));
    }
    if (state.sort === 'asc') list = [...list].sort((a, b) => a.price - b.price);
    if (state.sort === 'desc') list = [...list].sort((a, b) => b.price - a.price);

    grid.innerHTML = list.length ? list.map(v => `
        <article class="v-card">
            <div>
                <div class="v-head">
                    <span class="v-code mono">${v.id}</span>
                    ${statusTag(v.status)}
                </div>
                <div class="v-visual">${vehicleIcon(v.type)}</div>
                <div class="v-name">${v.name}</div>
                <div class="v-meta">${v.type} • Instant Pickup</div>
            </div>
            <div class="v-foot">
                <div class="v-rate"><strong class="mono">${v.price.toFixed(0)} BDT</strong> <small>/ day</small></div>
                <button type="button" class="btn btn-brand" style="width:auto; padding:8px 14px; font-size:0.82rem;" onclick="openCheckout('${v.id}','${v.name}',${v.price})">Book</button>
            </div>
        </article>
    `).join('') : `<div class="panel"><p style="color:var(--muted);">No matching available vehicles.</p></div>`;
}

function openCheckout(id, name, rate) {
    state.booking = { id, name, rate, days: 0, total: 0, method: 'bKash' };
    const overlay = document.getElementById('modalOverlay');
    overlay.classList.remove('hidden');

    overlay.innerHTML = `
        <div class="modal-sheet">
            <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:6px;">
                <h2 style="font-size:1.2rem; font-weight:800;">${name}</h2>
                <span class="v-code mono">${id}</span>
            </div>
            <p style="color:var(--muted); font-size:0.84rem; margin-bottom:1.15rem;">Rate: <strong class="mono">${rate.toFixed(0)} BDT</strong> / day</p>
            <form onsubmit="submitBooking(event)">
                <div class="row-2">
                    <div class="field"><label>Start Date</label><input type="date" id="bStart" required onchange="recalcFare()"></div>
                    <div class="field"><label>End Date</label><input type="date" id="bEnd" required onchange="recalcFare()"></div>
                </div>
                <div class="fare-slip">
                    <div class="fare-row"><span>Duration</span><span class="mono" id="fDays">0 days</span></div>
                    <div class="fare-row"><span>Daily Rate</span><span class="mono">${rate.toFixed(0)} BDT</span></div>
                    <div class="fare-total"><span>Total Payable</span><span class="mono" id="fTotal">0 BDT</span></div>
                </div>
                <div class="field">
                    <label>Payment Method</label>
                    <div class="pay-grid">
                        ${['bKash', 'Nagad', 'Credit Card', 'Cash on Delivery'].map(m => `
                            <button type="button" class="pay-btn ${state.booking.method === m ? 'active' : ''}" onclick="pickMethod('${m}', this)">${m}</button>
                        `).join('')}
                    </div>
                </div>
                <div style="display:flex; gap:10px; margin-top:1.2rem;">
                    <button type="submit" class="btn btn-brand">Pay & Book</button>
                    <button type="button" class="btn btn-ghost" onclick="closeModal()">Close</button>
                </div>
            </form>
        </div>
    `;
}

function closeModal() {
    document.getElementById('modalOverlay').classList.add('hidden');
    state.booking = null;
}

function pickMethod(m, btn) {
    state.booking.method = m;
    document.querySelectorAll('.pay-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
}

function recalcFare() {
    const s = new Date(document.getElementById('bStart').value);
    const e = new Date(document.getElementById('bEnd').value);
    if (!isNaN(s) && !isNaN(e) && e >= s) {
        const days = Math.floor((e - s) / 86400000) + 1;
        state.booking.days = days;
        state.booking.total = days * state.booking.rate;
    } else {
        state.booking.days = 0;
        state.booking.total = 0;
    }
    document.getElementById('fDays').textContent = `${state.booking.days} ${state.booking.days === 1 ? 'day' : 'days'}`;
    document.getElementById('fTotal').textContent = `${state.booking.total.toLocaleString()} BDT`;
}

async function submitBooking(e) {
    e.preventDefault();
    if (!state.booking || state.booking.total <= 0) {
        toast('Choose valid start and end dates', 'error');
        return;
    }
    const res = await postForm('/api/rentals/book', {
        email: state.user.email,
        vehicle_id: state.booking.id,
        start_date: document.getElementById('bStart').value,
        end_date: document.getElementById('bEnd').value,
        payment_method: state.booking.method,
        total_cost: state.booking.total
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        closeModal();
        loadUserSection();
    }
}

async function returnRental(rental_id, vehicle_id) {
    const res = await postForm('/api/rentals/return', { rental_id, vehicle_id });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') loadUserSection();
}

async function cancelRental(rental_id, vehicle_id) {
    const res = await postForm('/api/rentals/cancel', { rental_id, vehicle_id });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') loadUserSection();
}

async function submitProfile(e) {
    e.preventDefault();
    const res = await postForm('/api/update-profile', {
        email: state.user.email,
        name: document.getElementById('pName').value.trim(),
        phone: document.getElementById('pPhone').value.trim(),
        new_pass: document.getElementById('pNewPass').value,
        current_pass: document.getElementById('pCurPass').value
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success' && res.user) {
        state.user = res.user;
        renderApp();
    }
}

// --- ADMIN WORKSPACE ---
async function loadAdminSection() {
    const root = document.getElementById('workspaceRoot');
    const aRes = await fetch('/api/admin/analytics');
    const aData = await aRes.json();

    const kpiHtml = `
        <div class="metrics-strip">
            <div class="metric-box"><span>Total Revenue</span><strong class="mono" style="color:var(--ok);">${Number(aData.revenue || 0).toLocaleString()} BDT</strong></div>
            <div class="metric-box"><span>Active Rentals</span><strong class="mono">${aData.active_rentals || 0}</strong></div>
            <div class="metric-box"><span>Total Fleet</span><strong class="mono" style="color:var(--brand);">${aData.total_vehicles || 0}</strong></div>
        </div>
    `;

    if (state.adminTab === 'analytics') {
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head">
                    <h2>Operations & Reporting</h2>
                    <a href="/api/export-csv" class="btn btn-ghost" style="text-decoration:none; font-size:0.82rem;" download="rentals_report.csv">Download CSV</a>
                </div>
                <p style="color:var(--ink-2); font-size:0.9rem;">All booking records persist in <span class="mono">rentals.txt</span> and audit events stream into <span class="mono">activity.log</span>.</p>
            </div>
        `;
    } else if (state.adminTab === 'vehicles') {
        const vRes = await fetch('/api/vehicles');
        const vData = await vRes.json();
        const vehicles = vData.vehicles || [];
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>Fleet Inventory</h2></div>
                <table>
                    <thead><tr><th>ID</th><th>Vehicle</th><th>Category</th><th>Rate/Day</th><th>Status</th><th>Quick Action</th></tr></thead>
                    <tbody>
                        ${vehicles.map(v => `
                            <tr>
                                <td class="mono"><strong>${v.id}</strong></td>
                                <td>${v.name}</td>
                                <td>${v.type}</td>
                                <td class="mono">${v.price.toFixed(0)} BDT</td>
                                <td>${statusTag(v.status)}</td>
                                <td style="display:flex; gap:6px; align-items:center;">
                                    <select onchange="quickStatus('${v.id}', this.value)" style="width:auto; padding:5px 8px; font-size:0.78rem;">
                                        <option value="Available" ${v.status === 'Available' ? 'selected' : ''}>Available</option>
                                        <option value="Rented" ${v.status === 'Rented' ? 'selected' : ''}>Rented</option>
                                        <option value="Under Maintenance" ${v.status === 'Under Maintenance' ? 'selected' : ''}>Under Maintenance</option>
                                    </select>
                                    <button type="button" class="btn btn-soft-bad" onclick="removeVehicle('${v.id}')">Delete</button>
                                </td>
                            </tr>
                        `).join('')}
                    </tbody>
                </table>
            </div>
        `;
    } else if (state.adminTab === 'users') {
        const uRes = await fetch('/api/users');
        const uData = await uRes.json();
        const users = uData.users || [];
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>Customers</h2></div>
                <table>
                    <thead><tr><th>Name</th><th>Email</th><th>Phone</th></tr></thead>
                    <tbody>
                        ${users.length ? users.map(u => `
                            <tr><td><strong>${u.name}</strong></td><td>${u.email}</td><td class="mono">${u.phone}</td></tr>
                        `).join('') : '<tr><td colspan="3" style="color:var(--muted);">No customers registered.</td></tr>'}
                    </tbody>
                </table>
            </div>
        `;
    } else if (state.adminTab === 'add') {
        root.innerHTML = kpiHtml + `
            <div class="panel" style="max-width:540px;">
                <div class="panel-head"><h2>Add Vehicle</h2></div>
                <form onsubmit="submitAddVehicle(event)">
                    <div class="row-2">
                        <div class="field"><label>ID</label><input type="text" id="vId" required placeholder="V105"></div>
                        <div class="field"><label>Category</label><input type="text" id="vType" required placeholder="Sedan"></div>
                    </div>
                    <div class="field"><label>Model Name</label><input type="text" id="vName" required placeholder="Honda Accord"></div>
                    <div class="row-2">
                        <div class="field"><label>Rate / Day (BDT)</label><input type="number" step="0.01" id="vPrice" required placeholder="4500"></div>
                        <div class="field">
                            <label>Status</label>
                            <select id="vStatus">
                                <option value="Available">Available</option>
                                <option value="Under Maintenance">Under Maintenance</option>
                                <option value="Rented">Rented</option>
                            </select>
                        </div>
                    </div>
                    <button type="submit" class="btn btn-brand" style="width:auto;">Save Vehicle</button>
                </form>
            </div>
        `;
    } else if (state.adminTab === 'logs') {
        const lRes = await fetch('/api/admin/logs');
        const lData = await lRes.json();
        const logs = lData.logs || [];
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>System Audit Log</h2></div>
                <div class="log-box mono">
                    ${logs.length ? logs.map(line => `<div>${line}</div>`).join('') : '<div>No log entries yet.</div>'}
                </div>
            </div>
        `;
    }
}

async function quickStatus(id, status) {
    const res = await postForm('/api/vehicles/update', { id, status });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') loadAdminSection();
}

async function removeVehicle(id) {
    const res = await postForm('/api/vehicles/delete', { id });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') loadAdminSection();
}

async function submitAddVehicle(e) {
    e.preventDefault();
    const res = await postForm('/api/vehicles/add', {
        id: document.getElementById('vId').value.trim(),
        type: document.getElementById('vType').value.trim(),
        name: document.getElementById('vName').value.trim(),
        price: document.getElementById('vPrice').value,
        status: document.getElementById('vStatus').value
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        state.adminTab = 'vehicles';
        loadAdminSection();
    }
}

renderAuth();
