const I18N = {
    en: {
        signIn: 'Sign In',
        register: 'Register',
        welcomeBack: 'Welcome back',
        signInSub: 'Sign in to book rides or open AdminOS.',
        email: 'Email',
        password: 'Password',
        continueBtn: 'Continue',
        createAccount: 'Create account',
        regSub: '6+ chars with letter, number & symbol.',
        fullName: 'Full Name',
        phone: 'Phone',
        secQ: 'Security Question',
        secA: 'Answer',
        signOut: 'Sign Out',
        fleet: 'Fleet',
        myRentals: 'My Rentals',
        profile: 'Profile',
        overview: 'Overview',
        vehicles: 'Vehicles',
        customers: 'Customers',
        addVehicle: 'Add Vehicle',
        allBookings: 'All Bookings',
        auditLog: 'Audit Log',
        availVehicles: 'Available Vehicles',
        activeBookings: 'Active Bookings',
        totalTrips: 'Total Trips',
        totalRevenue: 'Total Revenue',
        activeRentals: 'Active Rentals',
        totalFleet: 'Total Fleet',
        book: 'Book',
        receipt: 'Receipt',
        extend: 'Extend',
        returnBtn: 'Return',
        cancelBtn: 'Cancel',
        editBtn: 'Edit',
        deleteBtn: 'Delete'
    },
    bn: {
        signIn: 'লগইন',
        register: 'রেজিস্টার',
        welcomeBack: 'স্বাগতম',
        signInSub: 'রাইড বুক করতে বা AdminOS খুলতে লগইন করুন।',
        email: 'ইমেইল',
        password: 'পাসওয়ার্ড',
        continueBtn: 'এগিয়ে যান',
        createAccount: 'অ্যাকাউন্ট তৈরি করুন',
        regSub: 'কমপক্ষে ৬ অক্ষর: অক্ষর, সংখ্যা ও বিশেষ চিহ্ন থাকতে হবে।',
        fullName: 'পুরো নাম',
        phone: 'ফোন নম্বর',
        secQ: 'নিরাপত্তা প্রশ্ন',
        secA: 'উত্তর',
        signOut: 'লগআউট',
        fleet: 'ফ্লিট',
        myRentals: 'আমার বুকিং',
        profile: 'প্রোফাইল',
        overview: 'ওভারভিউ',
        vehicles: 'গাড়ি সমূহ',
        customers: 'কাস্টমার',
        addVehicle: 'গাড়ি যোগ করুন',
        allBookings: 'সব বুকিং',
        auditLog: 'অডিট লগ',
        availVehicles: 'উপলব্ধ গাড়ি',
        activeBookings: 'সক্রিয় বুকিং',
        totalTrips: 'মোট ট্রিপ',
        totalRevenue: 'মোট আয়',
        activeRentals: 'সক্রিয় রেন্টাল',
        totalFleet: 'মোট ফ্লিট',
        book: 'বুক করুন',
        receipt: 'রসিদ',
        extend: 'বাড়ান',
        returnBtn: 'ফেরত দিন',
        cancelBtn: 'বাতিল',
        editBtn: 'এডিট',
        deleteBtn: 'ডিলিট'
    }
};

const DEFAULT_PHOTOS = {
    Sedan: [
        'https://images.unsplash.com/photo-1549317661-bd32c8ce0db2?auto=format&fit=crop&w=900&q=75',
        'https://images.unsplash.com/photo-1550355291-bbee04a92027?auto=format&fit=crop&w=900&q=75'
    ],
    SUV: [
        'https://images.unsplash.com/photo-1519641471654-76ce0107ad1b?auto=format&fit=crop&w=900&q=75',
        'https://images.unsplash.com/photo-1533473359331-0135ef1b58bf?auto=format&fit=crop&w=900&q=75'
    ],
    Bike: [
        'https://images.unsplash.com/photo-1558981403-c5f9899a28bc?auto=format&fit=crop&w=900&q=75',
        'https://images.unsplash.com/photo-1568772585407-9361bd9bf3a8?auto=format&fit=crop&w=900&q=75'
    ]
};

const state = {
    theme: localStorage.getItem('dc_theme') || 'light',
    lang: localStorage.getItem('dc_lang') || 'en',
    authTab: 'login',
    user: null,
    isAdmin: false,
    userTab: 'fleet',
    adminTab: 'analytics',
    vehicles: [],
    cardSlide: {},
    draftPhotos: [],
    userRentals: [],
    allRentals: [],
    category: 'All',
    search: '',
    sort: 'default',
    lockoutQuestion: '',
    booking: null
};

document.body.setAttribute('data-theme', state.theme);

function t(key) {
    return (I18N[state.lang] && I18N[state.lang][key]) || I18N.en[key] || key;
}

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
    if (state.user || state.isAdmin) renderApp();
    else renderAuth();
}

function toggleLang() {
    state.lang = state.lang === 'en' ? 'bn' : 'en';
    localStorage.setItem('dc_lang', state.lang);
    if (state.user || state.isAdmin) renderApp();
    else renderAuth();
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

function getVehiclePhotos(v) {
    if (Array.isArray(v.images) && v.images.length > 0) return v.images;
    const typeKey = Object.keys(DEFAULT_PHOTOS).find(
        k => k.toLowerCase() === (v.type || '').toLowerCase()
    );
    return DEFAULT_PHOTOS[typeKey] || DEFAULT_PHOTOS.Sedan;
}

function statusTag(status) {
    if (status === 'Available' || status === 'Returned') return `<span class="tag tag-ok">${status}</span>`;
    if (status === 'Under Maintenance') return `<span class="tag tag-warn">${status}</span>`;
    return `<span class="tag tag-bad">${status}</span>`;
}

function topUtilityButtons() {
    return `
        <button type="button" class="btn btn-ghost" style="padding:7px 11px; font-size:0.78rem;" onclick="toggleLang()">${state.lang === 'en' ? 'বাংলা' : 'EN'}</button>
        <button type="button" class="btn btn-ghost" style="padding:7px 11px; font-size:0.78rem;" onclick="toggleTheme()">${state.theme === 'dark' ? '☀️' : '🌙'}</button>
    `;
}

// --- Multi-Photo Helper Functions (Admin Upload + URLs) ---
function compressImageFile(file, maxW = 720, quality = 0.72) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => {
            const img = new Image();
            img.onload = () => {
                const scale = Math.min(1, maxW / img.width);
                const canvas = document.createElement('canvas');
                canvas.width = Math.max(1, Math.round(img.width * scale));
                canvas.height = Math.max(1, Math.round(img.height * scale));
                const ctx = canvas.getContext('2d');
                ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
                resolve(canvas.toDataURL('image/jpeg', quality));
            };
            img.onerror = reject;
            img.src = reader.result;
        };
        reader.onerror = reject;
        reader.readAsDataURL(file);
    });
}

async function handlePhotoFiles(inputEl, stripId) {
    const files = Array.from(inputEl.files || []);
    if (!files.length) return;
    for (const file of files) {
        if (!file.type.startsWith('image/')) continue;
        try {
            const dataUrl = await compressImageFile(file);
            state.draftPhotos.push(dataUrl);
        } catch {
            toast('Could not process one image', 'error');
        }
    }
    inputEl.value = '';
    renderDraftPhotoStrip(stripId);
}

function addPhotoUrlsFromInput(inputId, stripId) {
    const el = document.getElementById(inputId);
    if (!el) return;
    const parts = el.value.split(/[\n,]+/).map(s => s.trim()).filter(Boolean);
    if (!parts.length) return;
    for (const url of parts) {
        state.draftPhotos.push(url);
    }
    el.value = '';
    renderDraftPhotoStrip(stripId);
}

function removeDraftPhoto(idx, stripId) {
    state.draftPhotos.splice(idx, 1);
    renderDraftPhotoStrip(stripId);
}

function renderDraftPhotoStrip(stripId) {
    const strip = document.getElementById(stripId);
    if (!strip) return;
    strip.innerHTML = state.draftPhotos.length
        ? state.draftPhotos.map((src, i) => `
            <div class="photo-thumb-wrap">
                <img src="${src}" alt="Preview ${i + 1}">
                <button type="button" class="photo-del-btn" onclick="removeDraftPhoto(${i}, '${stripId}')" title="Remove">×</button>
            </div>
        `).join('')
        : `<span style="color:var(--muted); font-size:0.78rem;">No custom photos added yet (default category photos will be used).</span>`;
}

// --- AUTH VIEW ---
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
                <p>Native C Winsock server paired with file-backed persistence, multi-photo vehicle galleries, instant trip pricing, receipts, and live audit logs.</p>
                <div class="spec-list">
                    <span class="spec-item">Multi-Photo Fleet</span>
                    <span class="spec-item">bKash / Nagad / Card / COD</span>
                    <span class="spec-item">Printable Receipts</span>
                    <span class="spec-item">Audit Log & CSV</span>
                </div>
            </div>
            <div class="mono" style="font-size:0.78rem; color:var(--muted);">localhost:8080 • File Storage Active</div>
        </section>
        <section class="auth-form-col">
            <div class="auth-box">
                <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:1rem; gap:8px;">
                    <div class="tabs-bar" style="margin-bottom:0;">
                        <button type="button" class="tab-btn ${state.authTab === 'login' ? 'active' : ''}" onclick="setAuthTab('login')">${t('signIn')}</button>
                        <button type="button" class="tab-btn ${state.authTab === 'register' ? 'active' : ''}" onclick="setAuthTab('register')">${t('register')}</button>
                    </div>
                    <div style="display:flex; gap:6px;">${topUtilityButtons()}</div>
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
        <h2 style="font-size:1.5rem; font-weight:800; margin-bottom:4px;">${t('welcomeBack')}</h2>
        <p style="color:var(--muted); font-size:0.88rem; margin-bottom:1.35rem;">${t('signInSub')}</p>
        <form onsubmit="submitLogin(event)">
            <div class="field">
                <label>${t('email')}</label>
                <input type="email" id="lEmail" required placeholder="you@example.com">
            </div>
            <div class="field">
                <label>${t('password')}</label>
                <input type="password" id="lPass" required placeholder="Enter password">
            </div>
            <button class="btn btn-brand" type="submit">${t('continueBtn')}</button>
        </form>
        ${state.lockoutQuestion ? `
            <div class="lockout-box">
                <div style="font-weight:800; font-size:0.88rem; color:var(--bad); margin-bottom:4px;">Account Locked — Security Reset</div>
                <div class="mono" style="font-size:0.82rem; margin-bottom:10px; background:var(--surface); padding:8px; border-radius:8px;">${state.lockoutQuestion}</div>
                <form onsubmit="submitReset(event)">
                    <div class="field"><label>${t('secA')}</label><input type="text" id="rAns" required></div>
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
        <h2 style="font-size:1.5rem; font-weight:800; margin-bottom:4px;">${t('createAccount')}</h2>
        <p style="color:var(--muted); font-size:0.88rem; margin-bottom:1.35rem;">${t('regSub')}</p>
        <form onsubmit="submitRegister(event)">
            <div class="row-2">
                <div class="field"><label>${t('fullName')}</label><input type="text" id="rName" required></div>
                <div class="field"><label>${t('phone')}</label><input type="text" id="rPhone" required></div>
            </div>
            <div class="field"><label>${t('email')}</label><input type="email" id="rEmail" required></div>
            <div class="field"><label>${t('password')}</label><input type="password" id="rPass" required></div>
            <div class="row-2">
                <div class="field"><label>${t('secQ')}</label><input type="text" id="rSecQ" required placeholder="First school?"></div>
                <div class="field"><label>${t('secA')}</label><input type="text" id="rSecA" required></div>
            </div>
            <button class="btn btn-brand" type="submit">${t('createAccount')}</button>
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

// --- APP SHELL ---
function renderApp() {
    document.getElementById('authView').classList.add('hidden');
    const appEl = document.getElementById('appView');
    appEl.classList.remove('hidden');

    const navItems = state.isAdmin
        ? [
            ['analytics', t('overview')],
            ['vehicles', t('vehicles')],
            ['bookings', t('allBookings')],
            ['users', t('customers')],
            ['add', t('addVehicle')],
            ['logs', t('auditLog')]
        ]
        : [
            ['fleet', t('fleet')],
            ['bookings', t('myRentals')],
            ['profile', t('profile')]
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
                ${topUtilityButtons()}
                <button type="button" class="btn btn-soft-bad" onclick="logout()">${t('signOut')}</button>
            </div>
        </header>
        <div class="content" id="workspaceRoot"></div>
    `;

    if (state.isAdmin) loadAdminSection();
    else loadUserSection();
}

function switchSection(key) {
    if (key === 'add') state.draftPhotos = [];
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
    state.userRentals = bRes.rentals || [];
    const rentals = state.userRentals;
    const availCount = state.vehicles.filter(v => v.status === 'Available').length;
    const activeCount = rentals.filter(r => r.status === 'Active').length;

    const kpiHtml = `
        <div class="metrics-strip">
            <div class="metric-box"><span>${t('availVehicles')}</span><strong class="mono">${availCount}</strong></div>
            <div class="metric-box"><span>${t('activeBookings')}</span><strong class="mono" style="color:var(--brand);">${activeCount}</strong></div>
            <div class="metric-box"><span>${t('totalTrips')}</span><strong class="mono">${rentals.length}</strong></div>
        </div>
    `;

    if (state.userTab === 'fleet') {
        root.innerHTML = kpiHtml + renderFleetControls() + `<div class="cards-grid" id="fleetCards"></div>`;
        renderFleetCards();
    } else if (state.userTab === 'bookings') {
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>${t('myRentals')}</h2></div>
                <table>
                    <thead><tr><th>ID</th><th>Vehicle</th><th>Period</th><th>Total</th><th>Gateway</th><th>Status</th><th>Action</th></tr></thead>
                    <tbody>
                        ${rentals.length ? rentals.map(r => `
                            <tr>
                                <td class="mono"><strong>${r.rental_id}</strong></td>
                                <td>${r.vehicle_name} <span class="mono" style="color:var(--muted);">(${r.vehicle_id})</span></td>
                                <td class="mono">${r.start_date} → ${r.end_date}</td>
                                <td class="mono"><strong>${Number(r.total_cost).toFixed(0)} BDT</strong></td>
                                <td>${r.payment_method}</td>
                                <td>${statusTag(r.status)}</td>
                                <td style="display:flex; gap:6px; flex-wrap:wrap;">
                                    <button type="button" class="btn btn-ghost" style="padding:6px 10px; font-size:0.78rem;" onclick="openReceiptById('${r.rental_id}')">${t('receipt')}</button>
                                    ${r.status === 'Active' ? `
                                        <button type="button" class="btn btn-soft-warn" onclick="openExtendModal('${r.rental_id}','${r.vehicle_id}','${r.end_date}')">${t('extend')}</button>
                                        <button type="button" class="btn btn-soft-ok" onclick="openReturnModal('${r.rental_id}','${r.vehicle_id}')">${t('returnBtn')}</button>
                                        <button type="button" class="btn btn-soft-bad" onclick="cancelRental('${r.rental_id}','${r.vehicle_id}')">${t('cancelBtn')}</button>
                                    ` : ''}
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
                <div class="panel-head"><h2>${t('profile')}</h2></div>
                <form onsubmit="submitProfile(event)">
                    <div class="row-2">
                        <div class="field"><label>${t('fullName')}</label><input type="text" id="pName" value="${state.user.name}" required></div>
                        <div class="field"><label>${t('phone')}</label><input type="text" id="pPhone" value="${state.user.phone}" required></div>
                    </div>
                    <div class="row-2">
                        <div class="field"><label>${t('email')}</label><input type="email" value="${state.user.email}" disabled></div>
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

function stepSlide(vehicleId, delta, total, e) {
    if (e) e.stopPropagation();
    const cur = state.cardSlide[vehicleId] || 0;
    state.cardSlide[vehicleId] = (cur + delta + total) % total;
    renderFleetCards();
}

function goSlide(vehicleId, idx, e) {
    if (e) e.stopPropagation();
    state.cardSlide[vehicleId] = idx;
    renderFleetCards();
}

function openPhotoGallery(vehicleId) {
    const v = state.vehicles.find(item => item.id === vehicleId);
    if (!v) return;
    const photos = getVehiclePhotos(v);
    const activeIdx = Math.min(state.cardSlide[vehicleId] || 0, photos.length - 1);
    const overlay = document.getElementById('modalOverlay');
    overlay.classList.remove('hidden');

    overlay.innerHTML = `
        <div class="modal-sheet" style="max-width:620px;">
            <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:10px;">
                <div>
                    <h2 style="font-size:1.2rem; font-weight:800;">${v.name}</h2>
                    <span class="mono" style="font-size:0.78rem; color:var(--muted);">${v.id} • ${v.type} • ${Number(v.price).toFixed(0)} BDT/day</span>
                </div>
                <button type="button" class="btn btn-ghost" style="padding:6px 12px;" onclick="closeModal()">✕</button>
            </div>
            <div style="border-radius:12px; overflow:hidden; background:var(--surface-alt); margin-bottom:12px; max-height:360px;">
                <img id="galleryMainImg" src="${photos[activeIdx]}" alt="${v.name}" style="width:100%; height:340px; object-fit:cover; display:block;">
            </div>
            <div class="photo-preview-strip" style="margin-bottom:14px;">
                ${photos.map((src, i) => `
                    <div class="photo-thumb-wrap" style="width:88px; height:64px; cursor:pointer; border-color:${i === activeIdx ? 'var(--brand)' : 'var(--line)'};" onclick="document.getElementById('galleryMainImg').src='${src}'">
                        <img src="${src}" alt="Photo ${i + 1}">
                    </div>
                `).join('')}
            </div>
            <div style="display:flex; gap:10px;">
                <button type="button" class="btn btn-brand" onclick="closeModal(); openCheckout('${v.id}','${v.name}',${v.price});">${t('book')} ${v.name}</button>
            </div>
        </div>
    `;
}

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

    grid.innerHTML = list.length ? list.map(v => {
        const photos = getVehiclePhotos(v);
        const idx = (state.cardSlide[v.id] || 0) % photos.length;
        return `
            <article class="v-card">
                <div>
                    <div class="v-head">
                        <span class="v-code mono">${v.id}</span>
                        ${statusTag(v.status)}
                    </div>
                    <div class="v-photo-stage" onclick="openPhotoGallery('${v.id}')" title="Click to view gallery">
                        <img src="${photos[idx]}" alt="${v.name}" loading="lazy">
                        <span class="v-photo-count mono">${idx + 1}/${photos.length}</span>
                        ${photos.length > 1 ? `
                            <button type="button" class="v-arrow prev" onclick="stepSlide('${v.id}', -1, ${photos.length}, event)">‹</button>
                            <button type="button" class="v-arrow next" onclick="stepSlide('${v.id}', 1, ${photos.length}, event)">›</button>
                            <div class="v-dots">
                                ${photos.map((_, d) => `<button type="button" class="v-dot ${d === idx ? 'active' : ''}" onclick="goSlide('${v.id}', ${d}, event)"></button>`).join('')}
                            </div>
                        ` : ''}
                    </div>
                    <div class="v-name">${v.name}</div>
                    <div class="v-meta">${v.type} • Instant Pickup</div>
                </div>
                <div class="v-foot">
                    <div class="v-rate"><strong class="mono">${Number(v.price).toFixed(0)} BDT</strong> <small>/ day</small></div>
                    <button type="button" class="btn btn-brand" style="width:auto; padding:8px 14px; font-size:0.82rem;" onclick="openCheckout('${v.id}','${v.name}',${v.price})">${t('book')}</button>
                </div>
            </article>
        `;
    }).join('') : `<div class="panel"><p style="color:var(--muted);">No matching available vehicles.</p></div>`;
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
            <p style="color:var(--muted); font-size:0.84rem; margin-bottom:1.15rem;">Rate: <strong class="mono">${Number(rate).toFixed(0)} BDT</strong> / day</p>
            <form onsubmit="submitBooking(event)">
                <div class="row-2">
                    <div class="field"><label>Start Date</label><input type="date" id="bStart" required onchange="recalcFare()"></div>
                    <div class="field"><label>End Date</label><input type="date" id="bEnd" required onchange="recalcFare()"></div>
                </div>
                <div class="fare-slip">
                    <div class="fare-row"><span>Duration</span><span class="mono" id="fDays">0 days</span></div>
                    <div class="fare-row"><span>Daily Rate</span><span class="mono">${Number(rate).toFixed(0)} BDT</span></div>
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
    const payload = {
        email: state.user.email,
        vehicle_id: state.booking.id,
        vehicle_name: state.booking.name,
        start_date: document.getElementById('bStart').value,
        end_date: document.getElementById('bEnd').value,
        payment_method: state.booking.method,
        total_cost: state.booking.total
    };
    const res = await postForm('/api/rentals/book', payload);
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        await loadUserSection();
        openReceiptModal({
            rental_id: res.rental_id || 'CONFIRMED',
            user_email: state.user.email,
            vehicle_id: payload.vehicle_id,
            vehicle_name: payload.vehicle_name,
            start_date: payload.start_date,
            end_date: payload.end_date,
            total_cost: payload.total_cost,
            payment_method: payload.payment_method,
            status: 'Active'
        });
    }
}

function openReceiptById(rentalId) {
    const rec = state.userRentals.find(r => r.rental_id === rentalId) || state.allRentals.find(r => r.rental_id === rentalId);
    if (rec) openReceiptModal(rec);
}

function openReceiptModal(r) {
    const overlay = document.getElementById('modalOverlay');
    overlay.classList.remove('hidden');
    overlay.innerHTML = `
        <div class="modal-sheet">
            <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:10px;">
                <h2 style="font-size:1.2rem; font-weight:800;">Booking Receipt</h2>
                ${statusTag(r.status)}
            </div>
            <div class="fare-slip">
                <div class="fare-row"><span>Rental ID</span><span class="mono"><strong>${r.rental_id}</strong></span></div>
                <div class="fare-row"><span>Customer</span><span>${r.user_email || (state.user ? state.user.email : '')}</span></div>
                <div class="fare-row"><span>Vehicle</span><span>${r.vehicle_name} (${r.vehicle_id})</span></div>
                <div class="fare-row"><span>Dates</span><span class="mono">${r.start_date} → ${r.end_date}</span></div>
                <div class="fare-row"><span>Gateway</span><span>${r.payment_method}</span></div>
                <div class="fare-total"><span>Total Paid</span><span class="mono">${Number(r.total_cost).toLocaleString()} BDT</span></div>
            </div>
            <div class="no-print" style="display:flex; gap:10px;">
                <button type="button" class="btn btn-brand" onclick="window.print()">Print Receipt</button>
                <button type="button" class="btn btn-ghost" onclick="closeModal()">Close</button>
            </div>
        </div>
    `;
}

function openExtendModal(rentalId, vehicleId, currentEnd) {
    const v = state.vehicles.find(item => item.id === vehicleId);
    const rate = v ? Number(v.price) : 2000;
    const overlay = document.getElementById('modalOverlay');
    overlay.classList.remove('hidden');
    overlay.innerHTML = `
        <div class="modal-sheet">
            <h2 style="font-size:1.15rem; font-weight:800; margin-bottom:6px;">Extend Trip — ${rentalId}</h2>
            <p style="color:var(--muted); font-size:0.84rem; margin-bottom:1rem;">Current End Date: <span class="mono">${currentEnd}</span> • Rate: <span class="mono">${rate} BDT/day</span></p>
            <form onsubmit="submitExtend(event, '${rentalId}', '${currentEnd}', ${rate})">
                <div class="field"><label>New End Date</label><input type="date" id="extEnd" min="${currentEnd}" required onchange="calcExtra('${currentEnd}', ${rate})"></div>
                <div class="fare-slip">
                    <div class="fare-row"><span>Extra Days</span><span class="mono" id="extDays">0</span></div>
                    <div class="fare-total"><span>Additional Charge</span><span class="mono" id="extCost">0 BDT</span></div>
                </div>
                <div style="display:flex; gap:10px;">
                    <button type="submit" class="btn btn-brand">Confirm Extension</button>
                    <button type="button" class="btn btn-ghost" onclick="closeModal()">Close</button>
                </div>
            </form>
        </div>
    `;
}

function calcExtra(currentEnd, rate) {
    const oldD = new Date(currentEnd);
    const newD = new Date(document.getElementById('extEnd').value);
    let days = 0;
    if (!isNaN(oldD) && !isNaN(newD) && newD > oldD) {
        days = Math.floor((newD - oldD) / 86400000);
    }
    document.getElementById('extDays').textContent = `${days}`;
    document.getElementById('extCost').textContent = `${(days * rate).toLocaleString()} BDT`;
    return { days, cost: days * rate };
}

async function submitExtend(e, rentalId, currentEnd, rate) {
    e.preventDefault();
    const newEnd = document.getElementById('extEnd').value;
    const { days, cost } = calcExtra(currentEnd, rate);
    if (days <= 0) {
        toast('Select a date after current end date', 'error');
        return;
    }
    const res = await postForm('/api/rentals/extend', {
        rental_id: rentalId,
        new_end_date: newEnd,
        extra_cost: cost
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        closeModal();
        loadUserSection();
    }
}

function openReturnModal(rentalId, vehicleId) {
    const overlay = document.getElementById('modalOverlay');
    overlay.classList.remove('hidden');
    overlay.innerHTML = `
        <div class="modal-sheet">
            <h2 style="font-size:1.15rem; font-weight:800; margin-bottom:6px;">Return Vehicle — ${rentalId}</h2>
            <p style="color:var(--muted); font-size:0.84rem; margin-bottom:1rem;">Add an optional condition or fuel note before returning.</p>
            <form onsubmit="submitReturn(event, '${rentalId}', '${vehicleId}')">
                <div class="field"><label>Return / Fuel Note (optional)</label><input type="text" id="retNote" placeholder="Full tank, clean interior"></div>
                <div style="display:flex; gap:10px;">
                    <button type="submit" class="btn btn-brand">Complete Return</button>
                    <button type="button" class="btn btn-ghost" onclick="closeModal()">Close</button>
                </div>
            </form>
        </div>
    `;
}

async function submitReturn(e, rentalId, vehicleId) {
    e.preventDefault();
    const note = document.getElementById('retNote').value.trim();
    const res = await postForm('/api/rentals/return', { rental_id: rentalId, vehicle_id: vehicleId, note });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        closeModal();
        loadUserSection();
    }
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
    const gw = aData.gateway || {};

    const kpiHtml = `
        <div class="metrics-strip">
            <div class="metric-box"><span>${t('totalRevenue')}</span><strong class="mono" style="color:var(--ok);">${Number(aData.revenue || 0).toLocaleString()} BDT</strong></div>
            <div class="metric-box"><span>${t('activeRentals')}</span><strong class="mono">${aData.active_rentals || 0}</strong></div>
            <div class="metric-box"><span>${t('totalFleet')}</span><strong class="mono" style="color:var(--brand);">${aData.total_vehicles || 0}</strong></div>
        </div>
    `;

    if (state.adminTab === 'analytics') {
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head">
                    <h2>Operations, Gateway Revenue & Reporting</h2>
                    <a href="/api/export-csv" class="btn btn-ghost" style="text-decoration:none; font-size:0.82rem;" download="rentals_report.csv">Download CSV</a>
                </div>
                <div class="gateway-grid">
                    <div class="gateway-card"><span>bKash</span><strong class="mono">${Number(gw.bKash || 0).toLocaleString()} BDT</strong></div>
                    <div class="gateway-card"><span>Nagad</span><strong class="mono">${Number(gw.Nagad || 0).toLocaleString()} BDT</strong></div>
                    <div class="gateway-card"><span>Credit Card</span><strong class="mono">${Number(gw['Credit Card'] || 0).toLocaleString()} BDT</strong></div>
                    <div class="gateway-card"><span>Cash on Delivery</span><strong class="mono">${Number(gw['Cash on Delivery'] || 0).toLocaleString()} BDT</strong></div>
                </div>
            </div>
        `;
    } else if (state.adminTab === 'vehicles') {
        const vRes = await fetch('/api/vehicles');
        const vData = await vRes.json();
        state.vehicles = vData.vehicles || [];
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>${t('vehicles')}</h2></div>
                <table>
                    <thead><tr><th>ID</th><th>Photo</th><th>Vehicle</th><th>Category</th><th>Rate/Day</th><th>Status</th><th>Quick Action</th></tr></thead>
                    <tbody>
                        ${state.vehicles.map(v => {
                            const photos = getVehiclePhotos(v);
                            return `
                                <tr>
                                    <td class="mono"><strong>${v.id}</strong></td>
                                    <td>
                                        <img src="${photos[0]}" alt="${v.name}" style="width:54px; height:38px; object-fit:cover; border-radius:6px; cursor:pointer;" onclick="openPhotoGallery('${v.id}')">
                                        <span class="mono" style="font-size:0.72rem; color:var(--muted);">(${photos.length})</span>
                                    </td>
                                    <td>${v.name}</td>
                                    <td>${v.type}</td>
                                    <td class="mono">${Number(v.price).toFixed(0)} BDT</td>
                                    <td>${statusTag(v.status)}</td>
                                    <td style="display:flex; gap:6px; align-items:center; flex-wrap:wrap;">
                                        <select onchange="quickStatus('${v.id}', this.value)" style="width:auto; padding:5px 8px; font-size:0.78rem;">
                                            <option value="Available" ${v.status === 'Available' ? 'selected' : ''}>Available</option>
                                            <option value="Rented" ${v.status === 'Rented' ? 'selected' : ''}>Rented</option>
                                            <option value="Under Maintenance" ${v.status === 'Under Maintenance' ? 'selected' : ''}>Under Maintenance</option>
                                        </select>
                                        <button type="button" class="btn btn-ghost" style="padding:6px 10px; font-size:0.78rem;" onclick="openEditVehicleModal('${v.id}')">${t('editBtn')}</button>
                                        <button type="button" class="btn btn-soft-bad" onclick="removeVehicle('${v.id}')">${t('deleteBtn')}</button>
                                    </td>
                                </tr>
                            `;
                        }).join('')}
                    </tbody>
                </table>
            </div>
        `;
    } else if (state.adminTab === 'bookings') {
        const rRes = await fetch('/api/admin/rentals');
        const rData = await rRes.json();
        state.allRentals = rData.rentals || [];
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>${t('allBookings')}</h2></div>
                <table>
                    <thead><tr><th>ID</th><th>Customer</th><th>Vehicle</th><th>Period</th><th>Total</th><th>Gateway</th><th>Status</th><th>Action</th></tr></thead>
                    <tbody>
                        ${state.allRentals.length ? state.allRentals.map(r => `
                            <tr>
                                <td class="mono"><strong>${r.rental_id}</strong></td>
                                <td>${r.user_email}</td>
                                <td>${r.vehicle_name} <span class="mono" style="color:var(--muted);">(${r.vehicle_id})</span></td>
                                <td class="mono">${r.start_date} → ${r.end_date}</td>
                                <td class="mono"><strong>${Number(r.total_cost).toFixed(0)} BDT</strong></td>
                                <td>${r.payment_method}</td>
                                <td>${statusTag(r.status)}</td>
                                <td style="display:flex; gap:6px; flex-wrap:wrap;">
                                    <button type="button" class="btn btn-ghost" style="padding:6px 10px; font-size:0.78rem;" onclick="openReceiptById('${r.rental_id}')">${t('receipt')}</button>
                                    ${r.status === 'Active' ? `
                                        <button type="button" class="btn btn-soft-ok" onclick="adminForceAction('/api/rentals/return', '${r.rental_id}', '${r.vehicle_id}')">${t('returnBtn')}</button>
                                        <button type="button" class="btn btn-soft-bad" onclick="adminForceAction('/api/rentals/cancel', '${r.rental_id}', '${r.vehicle_id}')">${t('cancelBtn')}</button>
                                    ` : ''}
                                </td>
                            </tr>
                        `).join('') : '<tr><td colspan="8" style="color:var(--muted);">No bookings recorded.</td></tr>'}
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
                <div class="panel-head"><h2>${t('customers')}</h2></div>
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
            <div class="panel" style="max-width:600px;">
                <div class="panel-head"><h2>${t('addVehicle')}</h2></div>
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
                    <div class="field">
                        <label>Upload Multiple Photos (from PC)</label>
                        <input type="file" accept="image/*" multiple onchange="handlePhotoFiles(this, 'addPhotoStrip')">
                    </div>
                    <div class="field">
                        <label>Or Add Image URLs (comma or newline separated)</label>
                        <div style="display:flex; gap:8px;">
                            <input type="text" id="addPhotoUrl" placeholder="https://.../front.jpg, https://.../interior.jpg">
                            <button type="button" class="btn btn-ghost" style="white-space:nowrap;" onclick="addPhotoUrlsFromInput('addPhotoUrl', 'addPhotoStrip')">+ Add URL</button>
                        </div>
                        <div class="photo-preview-strip" id="addPhotoStrip"></div>
                    </div>
                    <button type="submit" class="btn btn-brand" style="width:auto;">Save Vehicle</button>
                </form>
            </div>
        `;
        renderDraftPhotoStrip('addPhotoStrip');
    } else if (state.adminTab === 'logs') {
        const lRes = await fetch('/api/admin/logs');
        const lData = await lRes.json();
        const logs = lData.logs || [];
        root.innerHTML = kpiHtml + `
            <div class="panel">
                <div class="panel-head"><h2>${t('auditLog')}</h2></div>
                <div class="log-box mono">
                    ${logs.length ? logs.map(line => `<div>${line}</div>`).join('') : '<div>No log entries yet.</div>'}
                </div>
            </div>
        `;
    }
}

function openEditVehicleModal(id) {
    const v = state.vehicles.find(item => item.id === id);
    if (!v) return;
    state.draftPhotos = Array.isArray(v.images) && v.images.length ? [...v.images] : [...getVehiclePhotos(v)];

    const overlay = document.getElementById('modalOverlay');
    overlay.classList.remove('hidden');
    overlay.innerHTML = `
        <div class="modal-sheet">
            <h2 style="font-size:1.15rem; font-weight:800; margin-bottom:1rem;">Edit Vehicle — ${v.id}</h2>
            <form onsubmit="submitEditVehicle(event, '${v.id}')">
                <div class="field"><label>Model Name</label><input type="text" id="eName" value="${v.name}" required></div>
                <div class="row-2">
                    <div class="field"><label>Category</label><input type="text" id="eType" value="${v.type}" required></div>
                    <div class="field"><label>Rate / Day (BDT)</label><input type="number" step="0.01" id="ePrice" value="${v.price}" required></div>
                </div>
                <div class="field">
                    <label>Status</label>
                    <select id="eStatus">
                        <option value="Available" ${v.status === 'Available' ? 'selected' : ''}>Available</option>
                        <option value="Rented" ${v.status === 'Rented' ? 'selected' : ''}>Rented</option>
                        <option value="Under Maintenance" ${v.status === 'Under Maintenance' ? 'selected' : ''}>Under Maintenance</option>
                    </select>
                </div>
                <div class="field">
                    <label>Upload More Photos (from PC)</label>
                    <input type="file" accept="image/*" multiple onchange="handlePhotoFiles(this, 'editPhotoStrip')">
                </div>
                <div class="field">
                    <label>Add Image URL(s)</label>
                    <div style="display:flex; gap:8px;">
                        <input type="text" id="editPhotoUrl" placeholder="https://.../side.jpg">
                        <button type="button" class="btn btn-ghost" style="white-space:nowrap;" onclick="addPhotoUrlsFromInput('editPhotoUrl', 'editPhotoStrip')">+ Add</button>
                    </div>
                    <div class="photo-preview-strip" id="editPhotoStrip"></div>
                </div>
                <div style="display:flex; gap:10px;">
                    <button type="submit" class="btn btn-brand">Save Changes</button>
                    <button type="button" class="btn btn-ghost" onclick="closeModal()">Close</button>
                </div>
            </form>
        </div>
    `;
    renderDraftPhotoStrip('editPhotoStrip');
}

async function submitEditVehicle(e, id) {
    e.preventDefault();
    const res = await postForm('/api/vehicles/update', {
        id,
        name: document.getElementById('eName').value.trim(),
        type: document.getElementById('eType').value.trim(),
        price: document.getElementById('ePrice').value,
        status: document.getElementById('eStatus').value,
        images: state.draftPhotos.join('^^^')
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        state.draftPhotos = [];
        closeModal();
        loadAdminSection();
    }
}

async function adminForceAction(url, rental_id, vehicle_id) {
    const res = await postForm(url, { rental_id, vehicle_id, note: 'Admin action' });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') loadAdminSection();
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
        status: document.getElementById('vStatus').value,
        images: state.draftPhotos.join('^^^')
    });
    toast(res.message, res.status === 'success' ? 'ok' : 'error');
    if (res.status === 'success') {
        state.draftPhotos = [];
        state.adminTab = 'vehicles';
        loadAdminSection();
    }
}

renderAuth();
