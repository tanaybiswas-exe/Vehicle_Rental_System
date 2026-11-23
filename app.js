let currentUser = null;
let allVehicles = [];
let currentCategory = 'All';
let selectedVehicleForBooking = null;
let selectedPaymentMethod = 'bKash';
let calculatedTotalValue = 0;

function toggleTheme() {
    const body = document.body;
    const current = body.getAttribute('data-theme');
    body.setAttribute('data-theme', current === 'dark' ? 'light' : 'dark');
}

function showToast(msg, type = 'success') {
    const toast = document.getElementById('toast');
    document.getElementById('toastMsg').textContent = msg;
    toast.className = `toast ${type} show`;
    setTimeout(() => toast.classList.remove('show'), 4000);
}

function togglePass(id, btn) {
    const el = document.getElementById(id);
    el.type = el.type === 'password' ? 'text' : 'password';
    btn.textContent = el.type === 'password' ? 'SHOW' : 'HIDE';
}

function encodeForm(data) {
    return Object.keys(data).map(k => encodeURIComponent(k) + '=' + encodeURIComponent(data[k])).join('&');
}

async function postAPI(url, data) {
    const res = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: encodeForm(data)
    });
    return res.json();
}

function switchAuthTab(tabId, btn) {
    ['userLogin', 'userReg'].forEach(id => document.getElementById(id).classList.add('hidden'));
    document.getElementById(tabId).classList.remove('hidden');
    document.querySelectorAll('.segmented-control button').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
}

function syncProfileUI() {
    if (!currentUser) return;
    document.getElementById('topUserName').textContent = currentUser.name;
    document.getElementById('userAvatar').textContent = currentUser.name.charAt(0).toUpperCase();
    document.getElementById('profName').textContent = currentUser.name;
    document.getElementById('profEmail').textContent = currentUser.email;
    document.getElementById('profPhone').textContent = currentUser.phone;

    document.getElementById('editProfName').value = currentUser.name;
    document.getElementById('editProfPhone').value = currentUser.phone;
    document.getElementById('editProfEmail').value = currentUser.email;
    document.getElementById('editProfNewPass').value = '';
    document.getElementById('editProfCurrentPass').value = '';
}

async function handleRegister(e) {
    e.preventDefault();
    const res = await postAPI('/api/register', {
        name: document.getElementById('regName').value,
        email: document.getElementById('regEmail').value,
        phone: document.getElementById('regPhone').value,
        password: document.getElementById('regPass').value,
        sec_q: document.getElementById('regSecQ').value,
        sec_a: document.getElementById('regSecA').value
    });
    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success') {
        e.target.reset();
        switchAuthTab('userLogin', document.querySelectorAll('.segmented-control button')[0]);
    }
}

async function handleUserLogin(e) {
    e.preventDefault();
    const email = document.getElementById('loginEmail').value.trim();
    const password = document.getElementById('loginPass').value;

    if (email.toLowerCase() === 'admin@rental.com') {
        const adminRes = await postAPI('/api/admin-login', { email, password });
        if (adminRes.status === 'success') {
            document.getElementById('authSection').classList.add('hidden');
            document.getElementById('adminDashboard').classList.remove('hidden');
            showToast('Admin session active.', 'success');
            loadAdminData();
            return;
        } else {
            showToast('Invalid admin credentials.', 'error');
            return;
        }
    }

    const res = await postAPI('/api/login', { email, password });
    if (res.status === 'success') {
        currentUser = res.user;
        document.getElementById('resetBox').classList.add('hidden');
        document.getElementById('authSection').classList.add('hidden');
        document.getElementById('userDashboard').classList.remove('hidden');
        syncProfileUI();
        showToast(`Welcome back, ${currentUser.name}!`, 'success');
        loadUserData();
    } else if (res.status === 'locked') {
        showToast(res.message, 'error');
        document.getElementById('secQuestionText').textContent = res.sec_q;
        document.getElementById('resetBox').classList.remove('hidden');
    } else {
        showToast(res.message, 'error');
    }
}

async function handleProfileUpdate(e) {
    e.preventDefault();
    const res = await postAPI('/api/update-profile', {
        email: currentUser.email,
        name: document.getElementById('editProfName').value.trim(),
        phone: document.getElementById('editProfPhone').value.trim(),
        new_pass: document.getElementById('editProfNewPass').value,
        current_pass: document.getElementById('editProfCurrentPass').value
    });
    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success' && res.user) {
        currentUser = res.user;
        syncProfileUI();
    }
}

async function handlePasswordReset(e) {
    e.preventDefault();
    const res = await postAPI('/api/reset-password', {
        email: document.getElementById('loginEmail').value,
        sec_a: document.getElementById('resetAnswer').value,
        new_pass: document.getElementById('resetNewPass').value,
        confirm_pass: document.getElementById('resetConfirmPass').value
    });
    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success') {
        document.getElementById('resetBox').classList.add('hidden');
        e.target.reset();
    }
}

function logout() {
    currentUser = null;
    document.getElementById('userDashboard').classList.add('hidden');
    document.getElementById('adminDashboard').classList.add('hidden');
    document.getElementById('authSection').classList.remove('hidden');
    showToast('Signed out safely.', 'success');
}

function showUserTab(tabId, title, sub, btn) {
    ['uAvail', 'uBookings', 'uProfile'].forEach(id => document.getElementById(id).classList.add('hidden'));
    document.getElementById(tabId).classList.remove('hidden');
    document.getElementById('userPageTitle').textContent = title;
    document.getElementById('userPageSub').textContent = sub;
    document.querySelectorAll('#userDashboard .nav-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    if (tabId === 'uAvail') loadUserData();
    if (tabId === 'uBookings') loadUserBookings();
    if (tabId === 'uProfile') syncProfileUI();
}

function showAdminTab(tabId, title, sub, btn) {
    ['aAnalytics', 'aVehicles', 'aUsers', 'aAdd', 'aUpdate'].forEach(id => document.getElementById(id).classList.add('hidden'));
    document.getElementById(tabId).classList.remove('hidden');
    document.getElementById('adminPageTitle').textContent = title;
    document.getElementById('adminPageSub').textContent = sub;
    document.querySelectorAll('#adminDashboard .nav-btn').forEach(b => b.classList.remove('active'));
    if (btn) btn.classList.add('active');
    loadAdminData();
}

function getEmoji(type) {
    const t = type.toLowerCase();
    if (t.includes('bike')) return '🏍️';
    if (t.includes('suv')) return '🚙';
    return '🚘';
}

function filterFleet(cat, btn) {
    currentCategory = cat;
    document.querySelectorAll('.filter-pill').forEach(p => p.classList.remove('active'));
    btn.classList.add('active');
    renderFleet();
}

function sortFleet() {
    const mode = document.getElementById('sortFleetSelect').value;
    if (mode === 'lowHigh') allVehicles.sort((a, b) => a.price - b.price);
    else if (mode === 'highLow') allVehicles.sort((a, b) => b.price - a.price);
    renderFleet();
}

function renderFleet() {
    const availList = allVehicles.filter(v => v.status === 'Available' &&
        (currentCategory === 'All' || v.type.toLowerCase() === currentCategory.toLowerCase()));

    document.getElementById('userCardsGrid').innerHTML = availList.map(v => `
        <div class="vehicle-card">
            <div>
                <div class="vc-top">
                    <span class="vc-id mono">${v.id}</span>
                    <span class="status-badge badge-avail">${v.status}</span>
                </div>
                <div class="vc-icon-box">${getEmoji(v.type)}</div>
                <div class="vc-title">${v.name}</div>
                <div class="vc-type">${v.type} Category</div>
            </div>
            <div class="vc-footer">
                <div class="vc-price"><strong class="mono">${v.price.toFixed(0)} BDT</strong> <span>/ day</span></div>
                <button class="btn-primary" style="width: auto; padding: 8px 15px; font-size: 0.82rem;" onclick="openBookingModal('${v.id}', '${v.name}', ${v.price})">Book & Pay</button>
            </div>
        </div>
    `).join('') || '<p style="color:var(--text-muted);">No available vehicles in this category.</p>';
}

async function loadUserData() {
    const res = await fetch('/api/vehicles');
    const data = await res.json();
    allVehicles = data.vehicles || [];

    const availList = allVehicles.filter(v => v.status === 'Available');
    document.getElementById('statUserAvail').textContent = availList.length;

    const bRes = await postAPI('/api/rentals/user', { email: currentUser.email });
    const activeB = (bRes.rentals || []).filter(r => r.status === 'Active');
    document.getElementById('statUserActiveBookings').textContent = activeB.length;

    renderFleet();
}

function openBookingModal(id, name, price) {
    selectedVehicleForBooking = { id, name, price };
    selectedPaymentMethod = 'bKash';
    document.querySelectorAll('.pay-option').forEach((el, idx) => {
        if (idx === 0) el.classList.add('selected');
        else el.classList.remove('selected');
    });

    document.getElementById('modalVehTitle').textContent = `Book: ${name} (${id})`;
    document.getElementById('modalVehId').value = id;
    document.getElementById('bookStartDate').value = '';
    document.getElementById('bookEndDate').value = '';
    document.getElementById('modalTotalCost').textContent = '0 BDT';
    document.getElementById('calcDaysCount').textContent = '0 days';
    document.getElementById('bookingModal').classList.remove('hidden');
}

function closeBookingModal() {
    document.getElementById('bookingModal').classList.add('hidden');
}

function selectPayment(method, el) {
    selectedPaymentMethod = method;
    document.querySelectorAll('.pay-option').forEach(p => p.classList.remove('selected'));
    el.classList.add('selected');
}

function calculateCost() {
    const start = new Date(document.getElementById('bookStartDate').value);
    const end = new Date(document.getElementById('bookEndDate').value);
    if (start && end && end >= start) {
        const diffTime = Math.abs(end - start);
        const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24)) + 1;
        calculatedTotalValue = diffDays * selectedVehicleForBooking.price;
        document.getElementById('modalTotalCost').textContent = `${calculatedTotalValue.toLocaleString()} BDT`;
        document.getElementById('calcDaysCount').textContent = `${diffDays} days`;
    } else {
        calculatedTotalValue = 0;
        document.getElementById('modalTotalCost').textContent = '0 BDT';
        document.getElementById('calcDaysCount').textContent = '0 days';
    }
}

async function confirmBooking(e) {
    e.preventDefault();
    if (calculatedTotalValue <= 0) {
        showToast('Please select valid rental dates.', 'error');
        return;
    }

    const res = await postAPI('/api/rentals/book', {
        email: currentUser.email,
        vehicle_id: selectedVehicleForBooking.id,
        start_date: document.getElementById('bookStartDate').value,
        end_date: document.getElementById('bookEndDate').value,
        payment_method: selectedPaymentMethod,
        total_cost: calculatedTotalValue
    });

    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success') {
        closeBookingModal();
        loadUserData();
    }
}

async function loadUserBookings() {
    const res = await postAPI('/api/rentals/user', { email: currentUser.email });
    const rentals = res.rentals || [];
    document.getElementById('userBookingsTable').innerHTML = rentals.map(r => `
        <tr>
            <td><strong class="mono">${r.rental_id}</strong></td>
            <td>${r.vehicle_name} <span class="mono" style="color:var(--text-muted);">(${r.vehicle_id})</span></td>
            <td class="mono">${r.start_date} → ${r.end_date}</td>
            <td><strong class="mono">${r.total_cost.toFixed(2)} BDT</strong></td>
            <td><span style="font-weight:700; color:var(--primary);">${r.payment_method || 'bKash'}</span></td>
            <td><span class="status-badge ${r.status === 'Active' ? 'badge-rented' : 'badge-avail'}">${r.status}</span></td>
            <td>
                ${r.status === 'Active' ? `<button class="btn-mini del" onclick="returnVehicle('${r.rental_id}', '${r.vehicle_id}')">Return</button>` : '<span style="color:var(--text-muted); font-size:0.82rem;">Completed</span>'}
            </td>
        </tr>
    `).join('') || '<tr><td colspan="7">No rental history found.</td></tr>';
}

async function returnVehicle(rental_id, vehicle_id) {
    if (!confirm('Confirm returning this vehicle?')) return;
    const res = await postAPI('/api/rentals/return', { rental_id, vehicle_id });
    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success') loadUserBookings();
}

async function loadAdminData() {
    const aRes = await fetch('/api/admin/analytics');
    const aData = await aRes.json();
    document.getElementById('statRevenue').textContent = `${aData.revenue.toLocaleString()} BDT`;
    document.getElementById('statActiveRentals').textContent = aData.active_rentals;
    document.getElementById('statTotalFleet').textContent = aData.total_vehicles;

    const vRes = await fetch('/api/vehicles');
    const vData = await vRes.json();
    document.getElementById('adminVehiclesTable').innerHTML = (vData.vehicles || []).map(v => `
        <tr>
            <td><strong class="mono">${v.id}</strong></td>
            <td>${v.name}</td>
            <td>${v.type}</td>
            <td><strong class="mono">${v.price.toFixed(2)} BDT</strong></td>
            <td><span class="status-badge ${v.status === 'Available' ? 'badge-avail' : (v.status === 'Rented' ? 'badge-rented' : 'badge-maint')}">${v.status}</span></td>
            <td><button class="btn-mini del" onclick="deleteVehicle('${v.id}')">Delete</button></td>
        </tr>
    `).join('');

    const uRes = await fetch('/api/users');
    const uData = await uRes.json();
    document.getElementById('adminUsersTable').innerHTML = (uData.users || []).map(u => `
        <tr><td><strong>${u.name}</strong></td><td>${u.email}</td><td class="mono">${u.phone}</td></tr>
    `).join('');
}

async function handleAddVehicle(e) {
    e.preventDefault();
    const res = await postAPI('/api/vehicles/add', {
        id: document.getElementById('addId').value.trim(),
        name: document.getElementById('addName').value.trim(),
        type: document.getElementById('addType').value.trim(),
        price: document.getElementById('addPrice').value,
        status: document.getElementById('addStatus').value
    });
    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success') { e.target.reset(); loadAdminData(); }
}

async function handleUpdateVehicle(e) {
    e.preventDefault();
    const res = await postAPI('/api/vehicles/update', {
        id: document.getElementById('updId').value.trim(),
        name: document.getElementById('updName').value.trim(),
        type: document.getElementById('updType').value.trim(),
        price: document.getElementById('updPrice').value,
        status: document.getElementById('updStatus').value
    });
    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success') { e.target.reset(); loadAdminData(); }
}

async function deleteVehicle(id) {
    if (!confirm(`Delete vehicle ${id}?`)) return;
    const res = await postAPI('/api/vehicles/delete', { id });
    showToast(res.message, res.status === 'success' ? 'success' : 'error');
    if (res.status === 'success') loadAdminData();
}