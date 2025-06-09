<script setup lang="ts">
import TheDesktop from './components/TheDesktop.vue';
import TheTaskbar from './components/TheTaskbar.vue';
</script>

<template>
  <div class="app-container">
    <TheDesktop />
    <TheTaskbar />
  </div>
</template>

<style scoped>
.app-container {
  width: 100vw;
  height: 100vh;
  background-color: #333;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}
</style>
